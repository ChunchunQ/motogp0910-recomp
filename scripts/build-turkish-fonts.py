#!/usr/bin/env python3
"""Add six missing Turkish glyphs to each decoded native font bank.

Keeps original BC3 blocks byte-for-byte. New glyphs occupy an appended atlas
half; all original vertical metrics are rescaled to preserve pixel geometry.
Uses existing letterforms/dot/cedilla; only the missing breve mark is drawn.
Requires Pillow >= 11.2 (DDS/BC3 encoder), and owner's decoded .font files.
"""
import argparse
import hashlib
import io
import json
import struct
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]

def dds_header(w, h, size):
    return b'DDS ' + struct.pack('<7I',124,0x81007,h,w,size,0,1) + bytes(44) + struct.pack('<II4s5I',32,4,b'DXT5',0,0,0,0,0) + struct.pack('<5I',0x1000,0,0,0,0)

def swapped(data):
    data = bytearray(data)
    data[0::2], data[1::2] = data[1::2], data[0::2]
    return data

def extend_font(bank, preview=None):
    assert struct.unpack_from('>I',bank)[0] == 1
    size, fmt, w, h = struct.unpack_from('>IIHH',bank,8)
    assert w == 1024 and h in (256,1024) and fmt == 0x201 and size == w*h+8
    start = 20+w*h
    assert struct.unpack_from('>I',bank,start)[0] == 368 and len(bank)==start+4+368*32
    metrics = [list(struct.unpack_from('>8f',bank,start+4+32*i)) for i in range(368)]
    image = Image.open(io.BytesIO(dds_header(w,h,w*h)+swapped(bank[20:start]))).convert('RGBA')
    def glyph(c):
        m=metrics[ord(c)-32]
        return image.crop((round(m[0]*w),round(m[2]*h),round(m[1]*w),round(m[3]*h)))
    def dot_parts(letter):
        # BC3 leaves faint alpha (7/255 in the small Impact face) across the
        # visual gap. Testing for *any* alpha joined dot and stem, duplicated
        # the whole i above İ, and erased the whole body of dotless ı.
        a=letter.getchannel('A');visible=a.point(lambda value:255 if value>=32 else 0)
        bb=visible.getbbox();assert bb
        rows=[a.crop((0,y,a.width,y+1)).getextrema()[1]>=32 for y in range(a.height)]
        end=next(y for y in range(bb[1]+1,a.height) if not rows[y])
        stem=next(y for y in range(end+1,a.height) if rows[y])
        assert end-bb[1]<letter.height/3 and stem<bb[3], 'Dot must be separate from the preserved stem'
        return bb[1],end,stem
    patch = Image.new('RGBA',(w,h))
    modified={}
    x=4
    for target,base in [('Ğ','G'),('ğ','g'),('İ','I'),('ı','i'),('Ş','S'),('ş','s')]:
        old=list(metrics[ord(base)-32]); letter=glyph(base); alpha=letter.getchannel('A'); bbox=alpha.getbbox()
        assert bbox
        above=below=0
        mark=None
        if target in 'Ğğ':
            # A shallow U, matching the original face's stem weight. Draw at
            # 4x resolution for antialiasing comparable to the source glyphs.
            mw=max(10,round((bbox[2]-bbox[0])*.62));mh=max(5,round(h*.009))
            mark=Image.new('RGBA',(mw*4,(mh+3)*4))
            draw=ImageDraw.Draw(mark);points=[(0,0),(mw*.15*4,mh*.60*4),(mw*.5*4,mh*4),(mw*.85*4,mh*.6*4),(mw*4,0)]
            draw.line(points,fill='white',width=max(6,round(h*.0035*4)),joint='curve')
            mark=mark.resize((mw,mh+3),Image.Resampling.LANCZOS)
            above=mark.height+max(2,round(h*.002))
        elif target=='İ':
            donor=glyph('i');top,end,_=dot_parts(donor)
            # The detached top component of the lowercase i is its native dot.
            mark=donor.crop((0,top,donor.width,end));mb=mark.getchannel('A').getbbox();mark=mark.crop(mb)
            above=mark.height+max(2,round(h*.003))
        elif target=='ı':
            _,_,stem=dot_parts(letter)
            ImageDraw.Draw(letter).rectangle((0,0,letter.width,stem-1),fill=(0,0,0,0))
            assert letter.getchannel('A').getbbox(), 'Dotless i must retain the letter body'
        else:
            donor=glyph('ç');db=donor.getchannel('A').getbbox()
            # ç's baseline-bearing delta identifies the cedilla even in the
            # capital Ç artwork, whose letter body was scaled to fit its cell.
            depth=max(3,round((metrics[ord('c')-32][6]-metrics[ord('ç')-32][6])*h))
            mark=donor.crop((0,db[3]-depth,donor.width,db[3]));mb=mark.getchannel('A').getbbox();assert mb;mark=mark.crop(mb)
            below=mark.height
        canvas=Image.new('RGBA',(max(letter.width,mark.width+6 if mark else 0),letter.height+above+below))
        dx=(canvas.width-letter.width)//2
        canvas.paste(letter,(dx,above))
        if mark is not None:
            # Center above/below the ink rather than the advance cell.
            mx=max(0,min(canvas.width-mark.width,dx+(bbox[0]+bbox[2]-mark.width)//2))
            my=bbox[1]+above-mark.height-max(2,round(h*.002)) if above else above+bbox[3]-1
            canvas.alpha_composite(mark,(mx,my))
        cellw=(canvas.width+7)//4*4;cellh=(canvas.height+7)//4*4
        assert x+cellw<w and cellh<h
        patch.alpha_composite(canvas,(x,4))
        # X advance and baseline are unchanged: growing UV height above/below
        # is offset by the same opposite bearing delta. Existing glyphs are
        # unaffected apart from normalized Y coordinates halving.
        new=list(old);new[:4]=[x/w,(x+canvas.width)/w,(h+4)/(2*h),(h+4+canvas.height)/(2*h)]
        new[4]-=dx/w;new[5]-=(canvas.width-letter.width-dx)/w
        new[6]=(old[6]-above/h)/2;new[7]=(old[7]-below/h)/2
        modified[target]=new
        x+=cellw+4
    encoded=io.BytesIO();patch.save(encoded,format='DDS',pixel_format='DXT5');payload=encoded.getvalue()[128:]
    assert len(payload)==w*h
    output=bytearray(bank[:20]);struct.pack_into('>I',output,8,2*w*h+8);struct.pack_into('>H',output,18,2*h)
    output+=bank[20:start]+swapped(payload)+struct.pack('>I',368)
    for i,m in enumerate(metrics):
        c=chr(i+32)
        if c in modified:n=modified[c]
        else:
            n=list(m)
            for j in (2,3,6,7):n[j]/=2
        # Prove the native horizontal advance and line height stay identical.
        before=((m[1]-m[0]+m[4]+m[5])*w,(m[3]-m[2]+m[6]+m[7])*h)
        expected=metrics[ord({'Ğ':'G','ğ':'g','İ':'I','ı':'i','Ş':'S','ş':'s'}.get(c,c))-32]
        advance=(n[1]-n[0]+n[4]+n[5])*w
        assert abs(advance-(expected[1]-expected[0]+expected[4]+expected[5])*w)<.001
        if c not in modified:assert abs((n[3]-n[2]+n[6]+n[7])*2*h-before[1])<.001
        output+=struct.pack('>8f',*n)
    assert output[20:start]==bank[20:start]
    if preview:
        # Developer inspection of the appended glyph row, decoded from the
        # actual BC3 bytes that will ship (not the uncompressed authoring canvas).
        restored=Image.open(io.BytesIO(dds_header(w,h,w*h)+payload)).convert('RGBA')
        restored.crop((0,0,x,128 if h==1024 else 64)).save(preview)
    return bytes(output)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--decoded-fonts',type=Path,required=True);p.add_argument('--preview-dir',type=Path);a=p.parse_args()
    destination=ROOT/'resources/localization/tr/overlay/fonts';destination.mkdir(parents=True,exist_ok=True)
    report=[]
    if a.preview_dir:a.preview_dir.mkdir(parents=True,exist_ok=True)
    for name in ('impact_17pt','impact_small','arial_bold_17pt','arial_bold_small'):
        original=(a.decoded_fonts/(name+'.font')).read_bytes()
        result=extend_font(original,a.preview_dir/(name+'.png') if a.preview_dir else None)
        (destination/(name+'.font')).write_bytes(result)
        report.append({'path':'fonts/'+name+'.font','source_decoded_sha256':hashlib.sha256(original).hexdigest(),'sha256':hashlib.sha256(result).hexdigest(),'size':len(result)})
    manifest=ROOT/'resources/localization/tr/manifest.json';data=json.loads(manifest.read_text());data['files']=[r for r in data['files'] if not r['path'].startswith('fonts/')]+report
    manifest.write_text(json.dumps(data,indent=2)+'\n')
    print('Built four native font banks; 24 Turkish glyphs; all original BC3 blocks preserved')
if __name__=='__main__':main()
