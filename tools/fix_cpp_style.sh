for file in $(find ./src/ -name '*.*')
do
    emacs -batch $file -l `pwd`/tools/cpp.el -f fixup
done


