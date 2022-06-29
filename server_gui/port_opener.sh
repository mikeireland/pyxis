ports="$(grep "port = " gui_ports_setup.toml | cut -d'=' -f2)"

rm temp.txt

RSAA_account=$"jhansen@motley.anu.edu.au"
IP=$"150.203.91.206"

for port in $ports; do
    echo "-L localhost:$port:$IP:$port " >> temp.txt
done

tr -d '\r' <temp.txt >temp.txt.new && mv temp.txt.new temp.txt
tr -d '\n' <temp.txt >temp.txt.new && mv temp.txt.new temp.txt

portoptions=$(<temp.txt)

command=$"ssh -N ${portoptions}${RSAA_account}"

eval $command
