clear
echo "---------- BUILB DOT SH ----------"
echo "progress: cleaning build dir"
make clean
echo "progress: build dir cleaned, starting build"
make -j$(nproc) | grep "error:"
echo "progress: build completed, checking for errors"
if [ $? -ne 0 ]; then
    echo "Build completed but probably had errors but probably not because this script is very broken."
    exit 1
else
    echo "Build completed successfully."
fi
echo "progress: build process finished"