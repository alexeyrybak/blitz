--TEST--
get last error with getError() 
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('{{ BEGIN }}');
echo 'getError():', $T->getError(), "\n";

?>
--EXPECTF--
invalid method call (noname_loaded_from_zval: line 1, pos 1)
seems that you forgot to close some begin/if/unless nodes with end (1 opened vs 0 end)
getError():SYNTAX ERROR: seems that you forgot to close some begin/if/unless nodes with end (1 opened vs 0 end)
