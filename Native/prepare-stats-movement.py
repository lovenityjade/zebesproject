from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text()
for name,id in [('FireUnchargedBeam',31),('FireChargedBeam',32),('FireHyperBeam',31)]:
 a=s.index('void '+name+'(void) {');b=s.index('\n}\n',a)
 body=s[a:b];old='      projectile_invincibility_timer = 10;';assert body.count(old)==1
 body=body.replace(old,f'      sm_stats_add({id});\n'+old);s=s[:a]+body+s[b:]
for old,new in [
 ('    projectile_invincibility_timer = 20;', '    sm_stats_add(hud_item_index==2?35:34);\n    projectile_invincibility_timer = 20;'),
 ('              power_bomb_flag = -1;', '              sm_stats_add(36);\n              power_bomb_flag = -1;'),
 ('      projectile_type[v1] = 1280;', '      sm_stats_add(37);\n      projectile_type[v1] = 1280;'),
 ('  samus_power_bombs = v2;\n  uint8 rv = kFireSbaFuncs', '  if(samus_power_bombs!=v2)sm_stats_add(33);\n  samus_power_bombs = v2;\n  uint8 rv = kFireSbaFuncs')]:
 assert s.count(old)==1,(old,s.count(old));s=s.replace(old,new)
p.write_text('#include "sm_run_stats.h"\n'+s)
