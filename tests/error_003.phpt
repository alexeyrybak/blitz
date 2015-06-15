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
invalid <if> syntax, probably a bracket mismatch but could be wrong operands too (/Users/nicolas/Documents/git/blitz/tests/error_003.tpl: line 1, pos 1)
invalid <if> syntax, probably a bracket mismatch but could be wrong operands too (/Users/nicolas/Documents/git/blitz/tests/error_003.tpl: line 3, pos 1)
invalid <if> syntax, probably a bracket mismatch but could be wrong operands too (/Users/nicolas/Documents/git/blitz/tests/error_003.tpl: line 6, pos 1)
seems that you forgot to close some begin/if/unless nodes with end (3 opened vs 2 end)
object(blitz)#1 (1) {
  ["tpl"]=>
  resource(5) of type (Blitz template)
}
Done
