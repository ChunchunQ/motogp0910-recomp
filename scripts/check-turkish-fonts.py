#!/usr/bin/env python3
"""Check shipped Turkish glyph bodies/metrics against the owner's native banks."""
import argparse,io,struct
from pathlib import Path
from PIL import Image,ImageChops,ImageDraw,ImageFont,ImageStat
from importlib.util import spec_from_file_location,module_from_spec
ROOT=Path(__file__).resolve().parents[1]
spec=spec_from_file_location('font_builder',ROOT/'scripts/build-turkish-fonts.py');builder=module_from_spec(spec);spec.loader.exec_module(builder)
PAIRS={'Ç':'C','ç':'c','Ğ':'G','ğ':'g','İ':'I','ı':'i','Ö':'O','ö':'o','Ş':'S','ş':'s','Ü':'U','ü':'u'}
def load(path):
 b=path.read_bytes();w,h=struct.unpack_from('>HH',b,16);end=20+w*h
 assert len(b)==end+4+368*32 and struct.unpack_from('>I',b,end)[0]==368
 image=Image.open(io.BytesIO(builder.dds_header(w,h,w*h)+builder.swapped(b[20:end]))).convert('RGBA')
 metrics={chr(i+32):struct.unpack_from('>8f',b,end+4+32*i) for i in range(368)}
 return b,w,h,image,metrics
def cell(font,c):
 _,w,h,image,metrics=font;a=metrics[c]
 return image.crop((round(a[0]*w),round(a[2]*h),round(a[1]*w),round(a[3]*h)))
def ink(im):return (im if im.mode=='L' else im.getchannel('A')).point(lambda a:255 if a>=128 else 0).getbbox()
def render(font,text):
 _,w,h,_,metrics=font;out=Image.new('RGBA',(1800,130))
 x=6
 for c in text:
  a=metrics[c];g=cell(font,c);out.alpha_composite(g,(round(x+a[4]*w),round(a[6]*h)+8));x+=(a[1]-a[0]+a[4]+a[5])*w
 return out.crop((0,0,min(out.width,round(x)+6),130))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--decoded-fonts',type=Path,required=True);p.add_argument('--overlay-fonts',type=Path,default=ROOT/'resources/localization/tr/overlay/fonts');p.add_argument('--preview-dir',type=Path);a=p.parse_args();checks=0
 sheets=[];titlefont=ImageFont.truetype(str(ROOT/'third_party/launcher-fonts/OpenSans-Regular.ttf'),15)
 for name in ('impact_17pt','impact_small','arial_bold_17pt','arial_bold_small'):
  old=load(a.decoded_fonts/(name+'.font'));new=load(a.overlay_fonts/(name+'.font'));_,w,h,_,om=old;_,nw,nh,_,nm=new
  assert nw==w and nh==2*h;assert old[0][20:20+w*h]==new[0][20:20+w*h];checks+=2
  for c,m in om.items():
   n=nm[c]
   if c not in PAIRS:
    expected=tuple(v/2 if j in (2,3,6,7) else v for j,v in enumerate(m));assert n==expected,(name,c,'unrelated glyph changed');checks+=1
  for c,base in PAIRS.items():
   m=om[base];n=nm[c];assert n[2]>=.5,(name,c,'not regenerated')
   assert abs((n[1]-n[0]+n[4]+n[5])*w-(m[1]-m[0]+m[4]+m[5])*w)<.002,(name,c,'advance')
   assert abs((n[3]-n[2]+n[6]+n[7])*nh-(m[3]-m[2]+m[6]+m[7])*h)<.002,(name,c,'line height')
   source=cell(old,base);actual=cell(new,c);dx=round((m[4]-n[4])*w);dy=round(m[6]*h-n[6]*nh)
   body=actual.crop((dx,dy,dx+source.width,dy+source.height));bb=ink(source);assert bb
   top=bb[1]
   if c=='ı':
    # The last vertical run is the stem, distinct from the detached dot.
    rows=[y for y in range(source.height) if source.getchannel('A').crop((0,y,source.width,y+1)).getextrema()[1]>=128]
    gaps=[right for left,right in zip(rows,rows[1:]) if right-left>1];assert gaps;top=gaps[-1]
    assert actual.getchannel('A').crop((dx,dy,dx+source.width,dy+top-1)).getextrema()[1]<128,(name,c,'dot remains')
   region=(bb[0],top,bb[2],bb[3]-1 if c in 'ÇçŞş' else bb[3])
   expected=source.getchannel('A').crop(region);observed=body.getchannel('A').crop(region)
   error=ImageStat.Stat(ImageChops.difference(expected,observed)).mean[0]
   assert error<12,(name,c,'body changed',error)
   assert ink(observed) and abs(ink(observed)[2]-ink(expected)[2])<=1,(name,c,'body width')
   assert ink(observed) and abs(ink(observed)[3]-ink(expected)[3])<=1,(name,c,'body height/baseline')
   if c in 'ÇçŞş':
    lower=actual.crop((0,dy+bb[3],actual.width,actual.height));tail=ink(lower);assert tail and tail[2]-tail[0]<.7*(bb[2]-bb[0]),(name,c,'oversized cedilla')
   else:assert round(n[6]*nh)+ink(actual)[1]>=0,(name,c,'accent clipped above line')
   checks+=7
  if a.preview_dir:
   a.preview_dir.mkdir(parents=True,exist_ok=True)
   lines=['CÇ cç GĞ gğ Iİ iı OÖ oö SŞ sş UÜ uü','BAŞLAMAK İÇİN START\'A BAS','ÇOK OYUNCULU  SEÇENEKLER  OYUNDAN ÇIK','Mod Seç   Seç']
   sheet=Image.new('RGB',(1800,28+len(lines)*130),(25,28,33));d=ImageDraw.Draw(sheet);d.text((5,4),name+' — actual shipped BC3 atlas and native bearings',font=titlefont,fill='#ffac30')
   for i,line in enumerate(lines):
    pic=render(new,line);sheet.paste(pic,(0,28+i*130),pic)
   sheet.save(a.preview_dir/(name+'-words.png'));sheets.append(sheet)
  print(name+': 12 full-size Turkish letter bodies, baselines, marks and native metrics verified')
 print(str(checks)+' Turkish font geometry / original-block checks passed')
if __name__=='__main__':main()
