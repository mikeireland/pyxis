#!/bin/bash
#Setup cron jobs for cleaning out the camera and platesolver directories
(crontab -l 2>/dev/null; echo -n "*/2 * * * * (cd ~/GitRepos/pyxis/servers/plate_solver/output && ls -tp | grep -v '/$' | tail -n +21 | xargs -d '\n' -r rm --)")|awk '!x[$0]++'|crontab -
(crontab -l 2>/dev/null; echo -n "*/2 * * * * (cd ~/GitRepos/pyxis/servers/coarse_star_tracker/data && ls -tp | grep -v '/$' | tail -n +11 | xargs -d '\n' -r rm --)")|awk '!x[$0]++'|crontab -

#Kill current screen
pkill screen
#Start terminals
screen -S DextraServers -c screen_config_dextra
