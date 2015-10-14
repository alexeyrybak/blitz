--TEST--
returning non-strings from user methods
--FILE---
<?php
include('common.inc');

$global_errors = '';

function myErrorHandler($errno, $errstr, $errfile, $errline) {
    global $global_errors;
    $global_errors .= $errstr;
}
set_error_handler("myErrorHandler");

class Tpl extends Blitz
{
    function get_array()  { return array(1,2,3); }
    function get_number() { return 2006; }
    function get_object() { return new Blitz(); }
}

$T = new Tpl();
$T->load("{{get_array()}} {{get_number()}} {{get_object()}}");
$body = $T->parse();

$ver = (int)phpversion();
if ($ver >= 7) {
    $test_errors = array(
        "Array to string conversion",
        "Object of class blitz could not be converted to string"
    );
} elseif($ver >= 5) {
    $test_errors = array(
        "Array to string conversion",
        "Object of class blitz could not be converted to string",
        "Object of class blitz to string conversion"
    );
} else {
    $test_errors = array(
        "Array to string conversion",
        "Object to string conversion");
}

if ($ver<=5) {
    if ($body === 'Array 2006 Object') {
        echo "body OK\n";
    }
} elseif ($ver >= 7) {
    if ($body === 'Array 2006 ') {
        echo "body OK\n";
    }
}

if ($global_errors === join($test_errors)) {
    echo "messages OK\n";
}

?>
--EXPECT--
body OK
messages OK
