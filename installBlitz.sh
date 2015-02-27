# Author: litechat@gmail.com

read -p "Specify the version of the project (example: blitz-0.8.13.tar.gz): " version
read -p "Specify the name of the php class (example: blitz13): " class

class1="${class,,}";
class2="${class^}";
class3="${class^^}";

wget "http://alexeyrybak.com/blitz/all-releases/$version"
tar zxvf $version
cd ${version%.*.*}

find . -exec rename "s/blitz/$class1/" '{}' \;
find . -exec rename "s/Blitz/$class2/" '{}' \;
find . -exec rename "s/BLITZ/$class3/" '{}' \;

grep -rl blitz . | xargs perl -pi~ -e "s/blitz/$class1/g"
grep -rl Blitz . | xargs perl -pi~ -e "s/Blitz/$class2/g"
grep -rl BLITZ . | xargs perl -pi~ -e "s/BLITZ/$class3/g"

phpize
./configure
make install

#echo "extension=$class1.so" >> "/etc/php5/conf.d/$class1.ini"
#/etc/init.d/apache2 restart
