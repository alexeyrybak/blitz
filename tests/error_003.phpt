--TEST--
broken templates 003
--FILE--
<?php

include('common.inc');
$T = new Blitz('error_003.tpl');

var_dump($T);

echo "Done\n";
?>
--EXPECTF--
invalid <if> syntax, wrong number of operands (%serror_003.tpl: line 1, pos 1)
invalid <if> syntax, probably a bracket mismatch but could be wrong operands too (%s: line 3, pos 1)
blitz::__construct(): Method calls in IF statements are only supported in form '{{IF callback(...)}}'. You cannot use method call results in statements or use nested method calls (%s: line 6, pos 1)
seems that you forgot to close some begin/if/unless nodes with end (3 opened vs 2 end)
object(blitz)#1 (1) {
  ["tpl"]=>
  resource(5) of type (Blitz template)
}
Done
