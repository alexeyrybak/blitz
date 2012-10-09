--TEST--
get last error with getError() 
--FILE--
<?php

$T = new Blitz();
$T->load('{{ BEGIN }}');
echo 'getError():', $T->getError();

?>
--EXPECTF--
Warning: blitz::load(): SYNTAX ERROR: invalid method call (noname_loaded_from_zval: line 1, pos 1) in %s on line 4
getError():SYNTAX ERROR: invalid method call (noname_loaded_from_zval: line 1, pos 1)
