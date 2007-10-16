--TEST--
get context
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('dummy');

echo $T->context('/some/context/')."\n";
echo $T->context('../where/am/i/../../')."\n";
echo $T->getContext()."\n";

$T->block('hello', array());
echo $T->getContext()."\n";

?>
--EXPECT--
/
/some/context
/some/where
/some/where
