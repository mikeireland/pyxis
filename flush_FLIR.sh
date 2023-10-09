#!/bin/bash

trap 'flush_cache $1 $2' EXIT

flush_cache(){
    cat ~/pyxis/screen_configs/pw | sudo -S 'ykushcmd ykush -s $1 -d $2'
    sleep 1
    cat ~/pyxis/screen_configs/pw | sudo -S 'ykushcmd ykush -s $1 -u $2'
}

source $3