--TEST--
expressions in conditional contexts
--FILE--
<?php

$T = new Blitz();
$T->load(
"{{ IF \$a1 >= \$b1 }}T1:YES{{ ELSE }}T1:NO{{ END }}
{{ IF \$a2 <= \$b2 }}T2:YES{{ ELSE }}T2:NO{{ END }}
{{ IF \$a3 == \$b3 }}T3:YES{{ ELSE }}T3:NO{{ END }}
{{ IF \$a4 == 0 }}T4:YES{{ ELSE }}T4:NO{{ END }}
{{ IF \$s5 == '' }}T5:YES{{ ELSE }}T5:NO{{ END }}
{{ IF \$s6 == 'hello' }}T6:YES{{ ELSE }}T6:NO{{ END }}
{{ IF \$s7 == 'hello, world!' }}T7:YES{{ ELSE }}T7:NO{{ END }}
{{ IF \$s8_1 != \$s8_2 }}T8:YES{{ ELSE }}T8:NO{{ END }}
{{ IF \$s9 == \$some.path.value }}T9:YES{{ ELSE }}T9:NO{{ END }}
");

$test_set = array(
    0 => array(
        'a1' => 3.35, 
        'b1' => 3.32,
        'b2' => 7,
        'a3' => -1,
        'b3' => -1,
        'a4' => 0.0,
        's5' => 'hello, world!',
        's6' => 'hello, world!',
        's7' => 'hello, world!',
        's8_1' => 'hello, world!',
        's8_2' => 'hello, world!',
        's9' => 'badoo!',
        'some' => array(
            'path' => array(
                'value' => 'badoo!'
            )
        )
    ), 
);

foreach ($test_set as $i_set) {
    $T->display($i_set);
}

unset($T);

$T = new Blitz();
$T->load(
"{{ BEGIN list }}\$_num = \"{{ \$_num }}\": {{ IF \$_num <= 1 }}<=1{{ ELSEIF \$_num>=4 }}>=4{{ ELSE }}...{{ END }}
{{ END }}
"
);

$i = 0;
$i_max = 9;

$set = array();
while ($i++ < $i_max) $set[] = array();

$T->display(array('list' => $set));

?>
--EXPECT--
T1:YES
T2:YES
T3:YES
T4:YES
T5:NO
T6:NO
T7:YES
T8:NO
T9:YES
$_num = "1": <=1
$_num = "2": ...
$_num = "3": ...
$_num = "4": >=4
$_num = "5": >=4
$_num = "6": >=4
$_num = "7": >=4
$_num = "8": >=4
$_num = "9": >=4

