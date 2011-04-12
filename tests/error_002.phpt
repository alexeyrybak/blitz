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
unexpected TAG_CLOSE (%serror_002.tpl: line 2, pos 12)
object(blitz)#%d (1) {
  ["tpl"]=>
  resource(%d) of type (Blitz template)
}
Done
