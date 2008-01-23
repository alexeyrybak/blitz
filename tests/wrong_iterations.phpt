--TEST--
iterate after wrong previous iterations 
--FILE--
<?php

include('common.inc');

$body = <<<END1
<!-- BEGIN A -->
  <!-- BEGIN B -->
  <!-- END B -->
<!-- END A -->
END1;

$body2 = <<<END2
<!-- BEGIN A -->{{ \$t }}<!-- END A -->
END2;

$T = new Blitz();
$T->load($body);

$T->set(array('A' => array('signin_url' => 1)));
$T->block('/A/B', array('signin_url' => 2));

$M = new Blitz();
$M->load($body2);

$M->set(array('A' => 'test'));
$M->block('/A', array('t' => 'test'));

?>
--EXPECT--
unable to iterate context "B" in "/A/B" because parent iteration was not set as array of arrays before. Correct your previous iteration sets.
unable to iterate context "A" in "/A" because it was set as "scalar" value before. Correct your previous iteration sets.
