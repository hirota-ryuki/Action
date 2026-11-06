"""pose_to_angles.py の結果をならす（3コマの中央値 → 1:2:1 の平均）。確かめ用に棒人間の絵も描く。
使い方: python clean_angles.py <角度.json> <出力.json> <棒人間.jpg>
赤 = A・a、青 = B・b。読み取りの乱れ（腕が逆向きに回るなど）が残ったら、出力の JSON を手で直す。
"""
import json, math, sys
import numpy as np, cv2
rows=json.load(open(sys.argv[1]))
def unwrap(v):
    out=[v[0]]
    for x in v[1:]:
        d=(x-out[-1]+180)%360-180; out.append(out[-1]+d)
    return out
def clean(v):
    v=unwrap(v); n=len(v)
    m=[sorted(v[max(0,i-1):i+2])[len(v[max(0,i-1):i+2])//2] for i in range(n)]  # median3
    s=[(m[max(0,i-1)]+2*m[i]+m[min(n-1,i+1)])/4 for i in range(n)]
    return s
ch={}
for side,key in (('A','A'),('B','B')):
    for k in ('thigh','shin','foot'): ch[side+k]=clean([r[key][k] for r in rows])
    ch['toe'+side]=clean([r['toe'+side] for r in rows])
for side in ('a','b'):
    for k in ('upper','fore'): ch[side+k]=clean([r[side][k] for r in rows])
ch['torso']=clean([r['torso'] for r in rows]); ch['neck']=clean([r['neck'] for r in rows]); ch['hipH']=clean([r['hipH'] for r in rows])
out=[{k:ch[k][i] for k in ch}|{'t':rows[i]['t']} for i in range(len(rows))]
json.dump(out,open(sys.argv[2],'w'),indent=0)
# stick figures
def P(o,a,l): return (o[0]+l*math.cos(math.radians(a)), o[1]-l*math.sin(math.radians(a)))
tiles=[]
for r in out:
    img=np.full((220,220,3),255,np.uint8); g=200
    hip=(110,g-r['hipH']*100)
    for s,col in (('B',(255,0,0)),('A',(0,0,255))):
        k=P(hip,r[s+'thigh'],50); a=P(k,r[s+'shin'],50); t=P(a,r[s+'foot'],18)
        for p,q in ((hip,k),(k,a),(a,t)): cv2.line(img,tuple(map(int,p)),tuple(map(int,q)),col,3)
    sh=P(hip,r['torso'],60); hd=P(sh,r['neck'],20)
    cv2.line(img,tuple(map(int,hip)),tuple(map(int,sh)),(0,0,0),3); cv2.circle(img,tuple(map(int,hd)),9,(0,0,0),2)
    for s,col in (('b',(255,0,0)),('a',(0,0,255))):
        e=P(sh,r[s+'upper'],32); w=P(e,r[s+'fore'],30)
        cv2.line(img,tuple(map(int,sh)),tuple(map(int,e)),col,2); cv2.line(img,tuple(map(int,e)),tuple(map(int,w)),col,2)
    cv2.line(img,(0,g),(220,g),(0,150,0),1); cv2.putText(img,'%.2f'%r['t'],(3,15),cv2.FONT_HERSHEY_SIMPLEX,0.45,(0,0,0),1)
    tiles.append(img)
while len(tiles)%10: tiles.append(np.full_like(tiles[0],255))
cv2.imwrite(sys.argv[3],np.vstack([np.hstack(tiles[j:j+10]) for j in range(0,len(tiles),10)]))
