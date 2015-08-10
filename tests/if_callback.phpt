--TEST--
test that callbacks work in ifs
--FILE--
<?php
include('common.inc');

error_reporting(E_ALL);
set_error_handler('my_error_handler');
function my_error_handler($errno, $errstr, $errfile, $errline) {
    echo "ERROR: " . rtrim($errstr) . "\n";
}

$body = <<<BODY
{{IF cb(1 > 5 && 2 < a, a, 1, 3 > 5)}}Callback works!{{END}}
Test between ifs
{{IF(cb("world", a, a > 5), "Inline callback also works!", "Does not work")}}
Text after ifs
{{IF empty_args_cb()}}Empty argument callback{{END}}
Text after empty args cb
{{IF(empty_args_cb(), "Inline empty arguments callback", "Something went wrong")}}
Text after all ifs
BODY;

class View extends Blitz {
	public function cb($a, $b, $c) {
		return $b == "hello";
	}

        public function empty_args_cb() {
            return func_get_args() === array();
        }
}

$T = new View();
$T->load($body);
$T->display(array('a' => "hello"));

?>
--EXPECT--
Callback works!
Test between ifs
Inline callback also works!
Text after ifs
Empty argument callback
Text after empty args cb
Inline empty arguments callback
Text after all ifs