# blitz

Read documentation at https://github.com/alexeyrybak/blitz/wiki 

## Installing blitz

```shell
tar zxvf blitz.tar.gz
cd blitz
phpize
./configure
make
make install
```

## Testing blitz

```shell
TEST_PHP_EXECUTABLE=/usr/local/bin/php php /your-cli-php-dir/run-tests.php -m tests/
```

You can use run-tests.sh script, but you need to change `TEST_PHP_EXECUTABLE`
path in this file first.

ATTENTION: following tests are bloken under PHP4:
* FAIL broken templates 001 [tests/error_001.phpt]
* FAIL broken templates 002 [tests/error_002.phpt]

You may just ignore these errors.
