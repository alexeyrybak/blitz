--TEST--
numerical and non-numerical keys in the iteration set
--FILE--
<?php
include('common.inc');

error_reporting(E_ALL);
set_error_handler('my_error_handler');
function my_error_handler($errno, $errstr, $errfile, $errline) {
    $parts = split('ERROR:',$errstr);
    echo trim($parts[1])."\n";
}

$T = new Blitz();
$T->load('
blabla
blabla {{ begin test }} test {{ end }}
blabla
');

$data = array(
   'test' => array(
       0 => array (),
       'num_photo_left' => 50,
       'me_edit_url' => 'http://192.168.3.4/058491/entry/37932/upload/1',
   )
);

$T->set($data);

echo $T->parse();

?>
--EXPECT--
You have a mix of numerical and non-numerical keys in the iteration set (context: test, line 3, pos 26), key was ignored
You have a mix of numerical and non-numerical keys in the iteration set (context: test, line 3, pos 26), key was ignored

blabla
blabla  test 
blabla
