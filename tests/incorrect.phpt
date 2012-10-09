--TEST--
incorrect code shouldn't break next correct one 
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load("test: <!-- _WHAT? -->{{var}}\n");
$T->display(array('var' => 'ok'));
?>
--EXPECT--
test: <!-- _WHAT? -->ok
