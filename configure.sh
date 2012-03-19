echo "Building symbolic links..."

ln -sf ./libGLEW.so.1.7.0 ./lib/linux/lin32/libGLEW.so.1.7
ln -sf ./libGLEW.so.1.7.0 ./lib/linux/lin32/libGLEW.so
ln -sf ./libglut.so.3.9.0 ./lib/linux/lin32/libglut.so
ln -sf ./libglut.so.3.9.0 ./lib/linux/lin32/libglut.so.3
ln -sf ./libAntTweakBar.so ./lib/linux/lin32/libAntTweakBar.so.1

ln -sf ./libGLEW.so.1.7.0 ./lib/linux/lin64/libGLEW.so.1.7
ln -sf ./libGLEW.so.1.7.0 ./lib/linux/lin64/libGLEW.so
ln -sf ./libglut.so.3.9.0 ./lib/linux/lin64/libglut.so
ln -sf ./libglut.so.3.9.0 ./lib/linux/lin64/libglut.so.3
ln -sf ./libAntTweakBar.so ./lib/linux/lin64/libAntTweakBar.so.1

echo "done!"

