clear
echo "---------- BUILB DOT SH ----------"

make clean
make -j24 | grep "error:"
if [ $? -ne 0 ]; then
    echo "Build failed with errors."
    exit 1
else
    echo "Build completed successfully."
fi