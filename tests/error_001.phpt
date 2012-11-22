--TEST--
broken templates 001 
--FILE--
<?php

include('common.inc');
$T = new Blitz('error_001.tpl');

var_dump($T);

echo "Done\n";
?>
--EXPECTF--	
invalid method call (%serror_001.tpl: line 1, pos 1)
invalid method call (%serror_001.tpl: line 2, pos 1)
invalid method call (%serror_001.tpl: line 3, pos 1)
seems that you forgot to close some begin/if/unless nodes with end (3 opened vs 0 end)
object(blitz)#%d (1) {
  ["tpl"]=>
  resource(%d) of type (Blitz template)
}
Done
