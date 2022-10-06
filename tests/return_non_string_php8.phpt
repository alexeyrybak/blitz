--TEST--
returning non-strings from user methods
--SKIPIF--
<?php
	if (PHP_VERSION_ID < 80000) die("SKIP: The test is for PHP v8.0+");
?>
--FILE---
<?php
include('common.inc');

$global_errors = '';

function myErrorHandler($errno, $errstr, $errfile, $errline) {
    global $global_errors;
    if (!empty($global_errors)) {
        $global_errors .= "\n";
    }
    $global_errors .= $errstr;
}
set_error_handler("myErrorHandler");

class Tpl extends Blitz
{
    function get_array()  { return array(1,2,3); }
    function get_number() { return 2006; }
    function get_object() { return new Blitz(); }
}


// number
$T = new Tpl();
$T->load("{{get_number()}}");
$body = $T->parse();
echo "$body\n";


// array
try {
    $T = new Tpl();
    $T->load("{{get_array()}}");
    $body = $T->parse();
    echo "$body\n";
} catch (\Throwable $t) {
    $class = get_class($t);
    echo "Exception caught: {$class} {$t->getMessage()}\n";
}

// array
try {
    $T = new Tpl();
    $T->load("{{get_object()}}");
    $body = $T->parse();
    echo "$body\n";
} catch (\Throwable $t) {
    $class = get_class($t);
    echo "Exception caught: {$class} {$t->getMessage()}\n";
}

echo "Errors:\n";
echo $global_errors;
?>
--EXPECT--
2006
Array
Exception caught: Error Object of class blitz could not be converted to string
Errors:
Array to string conversion