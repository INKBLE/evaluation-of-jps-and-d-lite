#!/usr/bin/env python3
"""Standard-library-only two-stage analysis for BenchmarkRunner CSV files."""
import argparse,csv,math,random,statistics
from collections import defaultdict
from pathlib import Path
H=['experiment_id','benchmark_type','map_family','grid_width','grid_height','obstacle_density','update_mode','planner','independent_map_count','candidate_map_count','solvable_map_count','solvability_rate','timing_repetitions','timing_statistic_per_map','median_elapsed_us','mean_elapsed_us','stddev_elapsed_us','q1_elapsed_us','q3_elapsed_us','p95_elapsed_us','median_expanded_nodes','mean_expanded_nodes','success_rate','notes']
def read(p):
 with open(p,encoding='utf-8',newline='') as f:return list(csv.DictReader(f))
def q(v,p):
 if not v:return 0.0
 v=sorted(v);x=(len(v)-1)*p;a=int(x);b=math.ceil(x);return v[a]+(v[b]-v[a])*(x-a)
def medians(rows,dynamic=False):
 g=defaultdict(list)
 for r in rows:
  if r['warmup']=='1' or r['success']!='1' or r['path_cost_matches_group']!='1':continue
  phase=r.get('phase','static')
  if dynamic and phase=='initial':continue
  g[(r['experiment_id'],r['map_family'],r['map_id'],r['map_hash'],r.get('update_mode',''),r['planner'],phase)].append(r)
 out=[]
 for k,v in g.items():out.append((k,statistics.median([float(x['elapsed_us']) for x in v]),statistics.median([float(x['expanded_nodes']) for x in v]),len(v),v[0]))
 return out
def manifest_stats(rows):
 z=defaultdict(lambda:[0,0])
 for r in rows:
  k=(r['map_family'],r['grid_width'],r['grid_height'],r['obstacle_density']);z[k][0]+=1;z[k][1]+=r['initially_solvable']=='1'
 return z
def summary(path,data,manifest,dynamic=False):
 groups=defaultdict(list);ms=manifest_stats(manifest)
 meta={(r['map_family'],r['map_id'],r['map_hash']):r for r in manifest}
 for k,t,x,n,r in data:
  exp,fam,mid,h,mode,planner,phase=k;source=meta.get((fam,mid,h),r);groups[(exp,fam,source.get('grid_width',''),source.get('grid_height',''),source.get('obstacle_density',''),mode,planner,phase)].append((t,x,n))
 with open(path,'w',encoding='utf-8',newline='') as f:
  w=csv.DictWriter(f,fieldnames=H);w.writeheader()
  for k,v in sorted(groups.items()):
   exp,fam,width,height,density,mode,planner,phase=k;ts=[x[0] for x in v];xs=[x[1] for x in v];cand,solv=ms[(fam,width,height,density)]
   w.writerow(dict(experiment_id=exp,benchmark_type='dynamic' if dynamic else 'static',map_family=fam,grid_width=width,grid_height=height,obstacle_density=density,update_mode=mode,planner=planner,independent_map_count=len(v),candidate_map_count=cand,solvable_map_count=solv,solvability_rate=solv/cand if cand else 0,timing_repetitions=v[0][2],timing_statistic_per_map='median_non_warmup_elapsed_us',median_elapsed_us=statistics.median(ts),mean_elapsed_us=statistics.mean(ts),stddev_elapsed_us=statistics.stdev(ts) if len(ts)>1 else 0,q1_elapsed_us=q(ts,.25),q3_elapsed_us=q(ts,.75),p95_elapsed_us=q(ts,.95),median_expanded_nodes=statistics.median(xs),mean_expanded_nodes=statistics.mean(xs),success_rate=1.0,notes=('phase='+phase if dynamic else 'conditional_on_solvable_and_correct')))
def paired(data,path,seed,resamples):
 g=defaultdict(dict)
 for k,t,_,_,r in data:
  exp,fam,mid,h,mode,planner,phase=k
  benchmark_type='dynamic' if phase!='static' else 'static'
  paired_phase='updated_solution' if benchmark_type=='dynamic' else phase
  g[(benchmark_type,exp,fam,r.get('grid_width',''),r.get('grid_height',''),r.get('obstacle_density',''),mode,paired_phase,mid,h)][planner]=t
 rows=[]
 for compare in ('JPS','DStarLite'):
  grouped=defaultdict(list)
  for k,x in g.items():
   if 'AStar'in x and compare in x and x[compare]>0:grouped[k[:-2]].append(x['AStar']/x[compare])
  for key,vals in sorted(grouped.items()):
   benchmark_type,exp,fam,width,height,density,mode,phase=key;rng=random.Random(seed+len(rows));boots=[statistics.median([vals[rng.randrange(len(vals))] for _ in vals]) for _ in range(resamples)]
   rows.append(dict(benchmark_type=benchmark_type,experiment_id=exp,map_family=fam,grid_width=width,grid_height=height,obstacle_density=density,update_mode=mode,phase=phase,compared_planner=compare,baseline_planner='AStar',paired_instances=len(vals),mean_speedup=statistics.mean(vals),median_speedup=statistics.median(vals),q1_speedup=q(vals,.25),q3_speedup=q(vals,.75),bootstrap_median_speedup_ci_low=q(boots,.025),bootstrap_median_speedup_ci_high=q(boots,.975),bootstrap_seed=seed,bootstrap_resamples=resamples))
 header=['benchmark_type','experiment_id','map_family','grid_width','grid_height','obstacle_density','update_mode','phase','compared_planner','baseline_planner','paired_instances','mean_speedup','median_speedup','q1_speedup','q3_speedup','bootstrap_median_speedup_ci_low','bootstrap_median_speedup_ci_high','bootstrap_seed','bootstrap_resamples']
 with open(path,'w',encoding='utf-8',newline='') as f:w=csv.DictWriter(f,fieldnames=header);w.writeheader();w.writerows(rows)
def main():
 p=argparse.ArgumentParser();p.add_argument('--input-dir',required=True);p.add_argument('--bootstrap-seed',type=int,default=20260807);p.add_argument('--bootstrap-resamples',type=int,default=10000);a=p.parse_args();d=Path(a.input_dir);m=read(d/'random_map_manifest.csv');meta={(r['map_family'],r['map_id'],r['map_hash']):r for r in m};s=medians(read(d/'static_raw.csv'));dy=medians(read(d/'dynamic_raw.csv'),True)
 for _,_,_,_,r in dy:r.update(meta.get((r['map_family'],r['map_id'],r['map_hash']),{}))
 summary(d/'static_summary.csv',s,m);summary(d/'dynamic_summary.csv',dy,m,True);paired(s+dy,d/'paired_comparisons.csv',a.bootstrap_seed,a.bootstrap_resamples)
if __name__=='__main__':main()