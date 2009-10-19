--TEST--
report duplicate contexts
--FILE--
<?php

include('common.inc');
ini_set('blitz.warn_context_duplicates', 1);

$T = new Blitz('duplicates_001.tpl');
echo "Done\n";
?>
--EXPECTF--	
context name "/test_dup/" duplicate in %sduplicates_001.tpl
context name "/test_dup2/" duplicate in %sduplicates_001.tpl
context name "/test_dup2/" duplicate in %sduplicates_001.tpl
Done
