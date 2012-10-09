--TEST--
expressions in conditional contexts
--FILE--
<?php

$T = new Blitz();
$T->load(
"*** Result: ***
{{ IF \$v == 'A'}}v = 'A'{{ ELSEIF \$v == 'B' }}v = 'B'{{ END }}
{{ IF \$v != 'A'}}v != 'A'{{ ELSEIF \$v != 'B' }}v != 'B'{{ END }}
");

$set = array(
    0 => array('G' => false, 'V' => false,),
    1 => array('G' => false, 'V' => array('v' => 'A')),
    2 => array('G' => false, 'V' => array('v' => 'B')),
    3 => array('G' => array('v' => 'A'), 'V => false'),
    4 => array('G' => array('v' => 'B'), 'V' => false),
);

foreach ($set as $s) {
    if (!empty($s['G']))
        $T->setGlobal($s['G']);
    if (!empty($s['V']))
        $T->set($s['V']);
    $T->display();
    $T->clean();
}

?>
--EXPECT--
*** Result: ***


*** Result: ***
v = 'A'
v != 'B'
*** Result: ***
v = 'B'
v != 'A'
*** Result: ***
v = 'A'
v != 'B'
*** Result: ***
v = 'B'
v != 'A'

