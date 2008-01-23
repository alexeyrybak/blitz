--TEST--
broken templates 002 
--FILE--
<?php

include('common.inc');
$T = new Blitz('error_002.tpl');

var_dump($T);

echo "Done\n";
?>
--EXPECTF--	
invalid method call (%serror_002.tpl: line 1, pos 1)
unexpected tag (%serror_002.tpl: line 2, pos 14)
object(blitz)#%d (1) {
  ["tpl"]=>
  resource(%d) of type (Blitz template)
}
Done
