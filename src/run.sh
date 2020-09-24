g++ -Wall -O2 -o hearts hearts.cc &&
./hearts $@ &&
echo &&
grep -Eo 'p0_1|p1_1|p2_1|p3_1' stats.txt | sort | uniq -c | awk '{print $2": "$1}'
