--TEST--
expressions with negative constant numbers
--FILE--
<?php
include('common.inc');

$list_vars = array(-1, -1.0, 0, 0.0, 1, 1.0);
$T = array();
foreach ($list_vars as $test)  {
    if (isset($T)) unset($T);
    $T = new Blitz();
    $T->load("{{ IF $test }}yes{{ ELSE }}no{{ END IF }}\n");
    $T->display(array('test'=>$test));
}
echo "####\n";
foreach ($list_vars as $test)  {
    if (isset($T)) unset($T);
    $T = new Blitz();
    $T->load("{{ IF $test >= 0 }}yes{{ ELSE }}no{{ END IF }}\n");
    $T->display(array('test'=>$test));
}

?>
--EXPECT--
yes
yes
no
no
yes
yes
####
no
no
yes
yes
yes
yes

