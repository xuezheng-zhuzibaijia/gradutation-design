for i in 10 100 1000 2000 5000 10000 20000 50000 100000
do 
    make clean;
    make all;
    ./writer $i >> result.txt;
done
