"""detect_pose.py の結果を、横から見た角度（前 = 画面の右 = 0°、真上 = 90°、真下 = -90°）に直す。
使い方: python pose_to_angles.py <detect_pose の出力.json> <出力.json>
A・B は脚（太もも・すね・足と、脚の長さを1としたつま先の高さ）、a・b は腕、torso は腰 → 肩、neck は肩 → 耳。
読み取りで左右の脚・腕が入れ替わったコマは、前のコマに近い方へそろえる。地面は、全コマで一番低い足の位置とする。
"""
import json, math, sys
res=json.load(open(sys.argv[1]))
def ang(a,b):
    return math.degrees(math.atan2(-(b[1]-a[1]), b[0]-a[0]))
def wrap(d): return (d+180)%360-180
def mid(a,b): return ((a[0]+b[0])/2,(a[1]+b[1])/2)
rows=[]
for r in res:
    p=r['pts']
    leg=lambda h,k,a,t:{'thigh':ang(p[h],p[k]),'shin':ang(p[k],p[a]),'foot':ang(p[a],p[t]),'toe_y':max(p[a][1],p[t][1]),'len':math.dist(p[h],p[k])+math.dist(p[k],p[a])}
    arm=lambda s,e,w:{'upper':ang(p[s],p[e]),'fore':ang(p[e],p[w])}
    hip=mid(p[23],p[24]); sh=mid(p[11],p[12]); ear=mid(p[7],p[8])
    rows.append({'t':r['t'],'A':leg(23,25,27,31),'B':leg(24,26,28,32),'a':arm(11,13,15),'b':arm(12,14,16),
                 'torso':ang(hip,sh),'neck':ang(sh,ear),'hip_y':hip[1]})
# continuity: legs
def dl(x,y): return sum(abs(wrap(x[k]-y[k])) for k in ('thigh','shin'))
def da(x,y): return sum(abs(wrap(x[k]-y[k])) for k in ('upper','fore'))
for i in range(1,len(rows)):
    pr,c=rows[i-1],rows[i]
    if dl(pr['A'],c['B'])+dl(pr['B'],c['A']) < dl(pr['A'],c['A'])+dl(pr['B'],c['B']): c['A'],c['B']=c['B'],c['A']
    if da(pr['a'],c['b'])+da(pr['b'],c['a']) < da(pr['a'],c['a'])+da(pr['b'],c['b']): c['a'],c['b']=c['b'],c['a']
ground=max(max(r['A']['toe_y'],r['B']['toe_y']) for r in rows)
L=sorted(r['A']['len'] for r in rows)[len(rows)//2]
for r in rows:
    r['hipH']=(ground-r['hip_y'])/L; r['toeA']=(ground-r['A']['toe_y'])/L; r['toeB']=(ground-r['B']['toe_y'])/L
json.dump(rows,open(sys.argv[2],'w'),indent=0)
print("legLen(px)",round(L),"ground",round(ground))
print(" t     torso neck | A thigh shin foot toe | B thigh shin foot toe | a up fore | b up fore | hipH")
for r in rows:
    A,B=r['A'],r['B']
    print("%.2f %5.0f %5.0f | %5.0f %5.0f %5.0f %4.2f | %5.0f %5.0f %5.0f %4.2f | %5.0f %5.0f | %5.0f %5.0f | %4.2f"%(r['t'],r['torso'],r['neck'],A['thigh'],A['shin'],A['foot'],r['toeA'],B['thigh'],B['shin'],B['foot'],r['toeB'],r['a']['upper'],r['a']['fore'],r['b']['upper'],r['b']['fore'],r['hipH']))
