# download
wget http://googletest.googlecode.com/files/gtest-1.7.0.zip
unzip gtest-1.7.0.zip

# build
cd gtest-1.7.0
./configure
make

# install
sudo cp -a include/gtest /usr/local/include
sudo cp -a lib/.libs/* /usr/local/lib/
