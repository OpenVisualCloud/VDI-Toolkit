#!/bin/bash
if [ $# = 0 ]
then
    echo "Usage: $0 host_file guest_file start_frame_num end_frame_num"
    exit
fi

#####input parameters####
host_file=$1
guest_file=$2
start_frame_num=$3
end_frame_num=$4

#####key string####
host_key_string=(
            "push back frame in host decoding service input queue"        #4
            "pop front frame in host decoding service input queue",       #5
            "begin avcodec send packet in FFmpeg decode service",         #6
            "complete avcodec receive frame in FFmpeg decode service",    #7
            "push back frame in host decoding service output queue",      #8
            "pop front frame in host decoding service output queue"       #9
            )
guest_key_string=(
            "begin send frame in sample decode",                          #1
            "push back frame in input queue in task data session",        #2
            "send gRPC frame buffer in VM",                               #3
            "receive gRPC frame buffer in VM",                            #10
            "pop front frame in output queue in task data session",       #11
            "complete receive frame in sample decode"                     #12
            )
#####host file analysis####
echo "host time analysis"
cur_index=0
for ((pts=start_frame_num; pts<=end_frame_num; pts++))
do
    string_index=(4 5 6 7 8 9) #host string 4 5 6 7 8 9
    for ((i = 0; i < ${#host_key_string[@]}; i++))
    do
        string=${host_key_string[i]}
        MATCHED_LINES=$(cat $host_file | awk "NR > $cur_index" | grep -m 1 -n "$string.*pts: $pts\b")
        curTime=$(echo "$MATCHED_LINES" | awk -F " " '{print $2}' | cut -d ':' -f 1-3)
        eval TimeStampT${string_index[$i]}[$pts]=$curTime
        #eval echo "T${string_index[$i]}[$pts] \${TimeStampT${string_index[$i]}[$pts]}"
    done
done

#####guest file analysis####
echo "guest time analysis"
cur_index=0
for ((pts=start_frame_num; pts<=end_frame_num; pts++))
do
    string_index=(1 2 3 10 11 12) #guest string 1 2 3 10 11 12
    for ((i = 0; i < ${#guest_key_string[@]}; i++))
    do
        string=${guest_key_string[i]}
        MATCHED_LINES=$(cat $guest_file | awk "NR > $cur_index" | grep -m 1 -n "$string.*pts: $pts\b")
        curTime=$(echo "$MATCHED_LINES" | awk -F " " '{print $2}' | cut -d ':' -f 1-3)
        eval TimeStampT${string_index[$i]}[$pts]=$curTime
        #eval echo "T${string_index[$i]}[$pts] \${TimeStampT${string_index[$i]}[$pts]}"
    done
done
#####summary latency####
# calc local time to totalTime
calc_time()
{
    totalTime=`date -d $1 +%s.%N`
    echo $totalTime
}

sumInterval=()
for ((i=1; i<=12; i++)); do
  sumInterval[$i]=0
done

for ((i=start_frame_num; i<=end_frame_num; i++))
do
    ## calculate each latency interval
    for ((j=1; j<=12; j++))
    do
        eval "time[$j]=\$(calc_time \${TimeStampT$j[$i]})"
        #echo "!!!!$j $i ${time[$j]}"
        if [ $j -gt 1 ]; then
            interval[$((j-1))]=$(bc -l <<< "${time[$j]}-${time[$((j-1))]}") #(1,11)
            #echo "interval[$((j-1))]: ${interval[$((j-1))]}"
            sumInterval[$((j-1))]=$(bc -l <<< "${sumInterval[$((j-1))]}+${interval[$((j-1))]}") #(1,11)
            #echo "sumInterval[$((j-1))]: ${sumInterval[$((j-1))]}"
            avgInterval[$((j-1))]=$(bc -l <<< "${sumInterval[$((j-1))]} / $((i-start_frame_num+1))") #(1,11)
            #echo "avgInterval[$((j-1))]: ${avgInterval[$((j-1))]}"
        fi
    done
done

avgE2EInterval=0

for num in "${avgInterval[@]}"; do
    avgE2EInterval=$(bc -l <<< "${avgE2EInterval}+${num}")
done
echo "each interval average: ${avgInterval[@]}"
echo "e2e average: ${avgE2EInterval}"