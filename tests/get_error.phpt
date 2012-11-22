--TEST--
get last error with getError() 
--FILE--
<?php

$T = new Blitz();
$T->load('{{ BEGIN }}');
echo 'getError():', $T->getError(), "\n";

?>
--EXPECTF--

Warning: blitz::load(): SYNTAX ERROR: invalid method call (noname_loaded_from_zval: line 1, pos 1) in %s on line 4

Warning: blitz::load(): SYNTAX ERROR: seems that you forgot to close some begin/if/unless nodes with end (1 opened vs 0 end) in %s on line 4
getError():SYNTAX ERROR: seems that you forgot to close some begin/if/unless nodes with end (1 opened vs 0 end)
