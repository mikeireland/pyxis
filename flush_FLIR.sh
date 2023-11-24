#!/bin/bash

trap 'flush_cache $1 $2' EXIT

flush_cache(){
    a=$1
    b=${a:1:1}
    if [ $b == "3" ]; then
        cat ~/pyxis/screen_configs/pw | sudo -S ykushcmd ykush3 -s $1 -d $2
        sleep 0.1
        cat ~/pyxis/screen_configs/pw | sudo -S ykushcmd ykush3 -s $1 -u $2
    else
        cat ~/pyxis/screen_configs/pw | sudo -S ykushcmd ykush -s $1 -d $2
        sleep 0.1
        cat ~/pyxis/screen_configs/pw | sudo -S ykushcmd ykush -s $1 -u $2
    fi
}

$3