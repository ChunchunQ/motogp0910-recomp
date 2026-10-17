#!/usr/bin/env python3
"""Normalize all twelve Turkish accented glyphs in each decoded native font bank.

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
    def ink(im, threshold=32):
        return im.getchannel('A').point(lambda value:255 if value>=threshold else 0).getbbox()
    def components(im):
        a=im.getchannel('A');points={(x,y) for y in range(im.height) for x in range(im.width) if a.getpixel((x,y))>=128};found=[]
        while points:
            seed=points.pop();queue=[seed];part=[seed]
            while queue:
                px,py=queue.pop()
                for q in ((px-1,py),(px+1,py),(px,py-1),(px,py+1)):
                    if q in points:points.remove(q);queue.append(q);part.append(q)
            if len(part)>2:found.append((len(part),(min(x for x,y in part),min(y for x,y in part),max(x for x,y in part)+1,max(y for x,y in part)+1)))
        return sorted(found,reverse=True)
    def dot_parts(letter):
        parts=components(letter);assert len(parts)>=2, 'Native i must have separate dot and stem'
        body=parts[0][1];dot=min((box for _,box in parts[1:] if box[3]<=body[1]),key=lambda box:box[1])
        return dot[1],dot[3],body[1]
    # Native Ç is a smaller, lower-case-shaped glyph, and native ç is an acute
    # glyph in these banks. Neither is a suitable letter body. Use ASCII C/c,
    # just as G/g/S/s are used, and borrow only the true cedilla below baseline.
    baseline=round(metrics[ord('C')-32][6]*h+ink(glyph('C'))[3])
    donor=glyph('Ç');donor_top=round(metrics[ord('Ç')-32][6]*h)
    tail=donor.crop((0,baseline-donor_top-1,donor.width,ink(donor)[3]))
    tail=tail.crop(ink(tail));assert tail.width<glyph('C').width*.7 and tail.height>=3
    # Lowercase ö has two isolated native dots even where compressed capitals
    # merge dot/stem alpha fringes. Reuse those marks, not i's wide rectangle.
    dia=glyph('ö');parts=components(dia);body=parts[0][1]
    dots=sorted((box for _,box in parts[1:] if box[3]<=body[1]),key=lambda box:box[0]);assert len(dots)==2
    dot=dots[0];single_dot=dia.crop((dot[0]-1,dot[1]-1,dot[2]+1,dot[3]+1))
    double_dot=dia.crop((dots[0][0]-1,min(b[1] for b in dots)-1,dots[1][2]+1,max(b[3] for b in dots)+1))
    cap_mark_top=round(metrics[ord('Ü')-32][6]*h+ink(glyph('Ü'),128)[1])
    low_mark_top=round(metrics[ord('ö')-32][6]*h+min(b[1] for b in dots))
    pairs={'Ç':'C','ç':'c','Ğ':'G','ğ':'g','İ':'I','ı':'i','Ö':'O','ö':'o','Ş':'S','ş':'s','Ü':'U','ü':'u'}
    patch=Image.new('RGBA',(w,h));modified={};body_copies=[];x=4
    for target,base in pairs.items():
        old=list(metrics[ord(base)-32]);letter=glyph(base);bbox=ink(letter);assert bbox
        top=round(old[6]*h);mark=None;mark_y=0;stem=0
        if target=='ı':
            _,_,stem=dot_parts(letter)
            ImageDraw.Draw(letter).rectangle((0,0,letter.width,stem-1),fill=(0,0,0,0))
            assert ink(letter), 'Dotless i must retain the complete native stem'
        elif target in 'ÇçŞş':
            mark=tail.copy();mark_y=top+bbox[3]-1
        elif target in 'İÖöÜü':
            mark=(single_dot if target=='İ' else double_dot).copy()
            desired=cap_mark_top if target.isupper() else low_mark_top
            mark_y=min(desired-ink(mark,128)[1],top+bbox[1]-1-ink(mark,128)[3])
        else:
            # Match the source cap/x-height and native accent line. A compact
            # antialiased breve is the sole mark absent from the original bank.
            mw=max(8,round((bbox[2]-bbox[0])*.62));mh=max(4,round((ink(glyph('G'))[3]-ink(glyph('G'))[1])*.13))
            stroke=max(2,round(mh*.4));up=4
            mark=Image.new('RGBA',(mw*up,mh*up));draw=ImageDraw.Draw(mark)
            pts=[(stroke*.5*up,stroke*.5*up),(mw*.23*up,(mh-stroke)*up),(mw*.5*up,(mh-stroke*.5)*up),(mw*.77*up,(mh-stroke)*up),((mw-stroke*.5)*up,stroke*.5*up)]
            draw.line(pts,fill='white',width=stroke*up,joint='curve');mark=mark.resize((mw,mh),Image.Resampling.LANCZOS)
            desired=cap_mark_top if target.isupper() else low_mark_top
            mark_y=min(desired,top+bbox[1]-mh-1)-ink(mark,128)[1]
        above=max(0,top-mark_y+3) if mark is not None else 0
        below=max(0,mark_y+mark.height+3-(top+letter.height)) if mark is not None else 0
        canvas=Image.new('RGBA',(max(letter.width,mark.width+6 if mark else 0),letter.height+above+below))
        dx=(canvas.width-letter.width)//2;canvas.paste(letter,(dx,above))
        if mark is not None:
            mx=dx+(bbox[0]+bbox[2]-mark.width)//2
            canvas.alpha_composite(mark,(mx,mark_y-top+above))
        cellw=(canvas.width+7)//4*4;cellh=(canvas.height+7)//4*4
        assert x+cellw<w and cellh<h
        # Preserve source BC3 block phase, then reuse whole original body blocks
        # that do not intersect a new mark (or the dot removed from dotless i).
        source_x,source_y=round(old[0]*w),round(old[2]*h)
        x+=(source_x-dx-x)%4;cell_y=4+(source_y-above)%4
        assert x+cellw<w
        patch.alpha_composite(canvas,(x,cell_y))
        mark_rect=(x+mx,cell_y+mark_y-top+above,x+mx+mark.width,cell_y+mark_y-top+above+mark.height) if mark is not None else None
        body_copies.append((source_x,source_y,letter.width,letter.height,x+dx,cell_y+above,mark_rect,stem))
        new=list(old);new[:4]=[x/w,(x+canvas.width)/w,(h+cell_y)/(2*h),(h+cell_y+canvas.height)/(2*h)]
        new[4]-=dx/w;new[5]-=(canvas.width-letter.width-dx)/w
        new[6]=(old[6]-above/h)/2;new[7]=(old[7]-below/h)/2
        modified[target]=new;x+=cellw+4
    encoded=io.BytesIO();patch.save(encoded,format='DDS',pixel_format='DXT5');payload=encoded.getvalue()[128:]
    assert len(payload)==w*h
    payload=bytearray(payload);native=swapped(bank[20:start])
    for sx,sy,sw,sh,tx,ty,mark,stem in body_copies:
        for by in range(sy//4,(sy+sh+3)//4):
            for bx in range(sx//4,(sx+sw+3)//4):
                px,py=tx+bx*4-sx,ty+by*4-sy
                if stem and by*4<sy+stem:continue
                if mark and px<mark[2] and px+4>mark[0] and py<mark[3] and py+4>mark[1]:continue
                assert px%4==0 and py%4==0 and 0<=px<w and 0<=py<h
                src=(by*(w//4)+bx)*16;dst=((py//4)*(w//4)+px//4)*16
                payload[dst:dst+16]=native[src:src+16]
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
        expected=metrics[ord(pairs.get(c,c))-32]
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
    print('Built four native font banks; 48 normalized Turkish glyphs; all original BC3 blocks preserved')
if __name__=='__main__':main()
