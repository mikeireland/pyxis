#!/bin/bash
#Setup cron jobs for cleaning out the camera and platesolver directories
#(crontab -l 2>/dev/null; echo -n "*/2 * * * * (cd ~/GitRepos/pyxis/servers/plate_solver/output && ls -tp | grep -v '/$' | tail -n +21 | xargs -d '\n' -r rm --)")|awk '!x[$0]++'|crontab -
#(crontab -l 2>/dev/null; echo -n "*/2 * * * * (cd ~/GitRepos/pyxis/servers/coarse_star_tracker/data && ls -tp | grep -v '/$' | tail -n +11 | xargs -d '\n' -r rm --)")|awk '!x[$0]++'|crontab -

#Kill current screen
pkill screen

#Start terminals
echo "Which servers to run?"
echo "n0 - Navis ALL"
echo "n1 - Navis ROBOT+AUX"
echo "n2 - Navis CAMERAS + PLATESOLVER"
echo "d0 - Dextra ALL"
echo "d1 - Dextra ROBOT+AUX"
echo "d2 - Dextra CAMERA/PLATESOLVER/TARGET"
echo "s0 - Sinistra ALL"
echo "s1 - Sinistra ROBOT+AUX"
echo "s2 - Sinistra CAMERA/PLATESOLVER/TARGET"

read command
case $command in
    n0) screen -L -S NavisServers -c screen_configs/screen_config_navis;;
    n1) screen -L -S NavisServersRobot -c screen_configs/screen_config_navis_robot;;
    n2) screen -L -S NavisServersCamera -c screen_configs/screen_config_navis_camera;;
    d0) screen -L -S DextraServers -c screen_configs/screen_config_dextra;;
    d1) screen -L -S DextraServersRobot -c screen_configs/screen_config_dextra_robot;;
    d2) screen -L -S DextraServersCamera -c screen_configs/screen_config_dextra_camera;;
    s0) screen -L -S SinistraServers -c screen_configs/screen_config_sinistra;;
    s1) screen -L -S SinistraServersRobot -c screen_configs/screen_config_sinistra_robot;;
    s2) screen -L -S SinistraServersCamera -c screen_configs/screen_config_sinistra_camera;;
    *) echo "INVALID COMMAND. EXITING";;
esac    

