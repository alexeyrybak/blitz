--TEST--
expressions in conditional contexts
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

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
{{ IF \$s10_1 == \$s10_2 }}T10:YES{{ ELSE }}T10:NO{{ END }}
{{ IF \$s11_1 == \$s11_2 }}T11:YES{{ ELSE }}T11:NO{{ END }}
{{ IF \$s12_1 >= \$s12_2 }}T12:YES{{ ELSE }}T12:NO{{ END }}
{{ IF \$s13_1 >= \$s13_2 }}T13:YES{{ ELSE }}T13:NO{{ END }}
{{ IF \$s14_1 >= \$s14_2 }}T14:YES{{ ELSE }}T14:NO{{ END }}
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
        's10_1' => true,
        's10_2' => false,
        's11_1' => false,
        's11_2' => false,
        's12_1' => -333,
        's12_2' => -334,
        's13_1' => 1234,
        's13_2' => 1235,
        's14_1' => 22071976,
        's14_2' => 22072011,
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

unset($T);

$T = new Blitz();
$T->load(
"{{ begin block }}
{{ if \$_num > 5 }}
Item {{ \$_num }} by condition \$_num > 5
{{ end }}
{{ end block }}
");

for ($i=0; $i<10; $i++) {
    $T->block('/block');
}
$T->display();

?>
--EXPECT--
T1:YES
T2:NO
T3:YES
T4:YES
T5:NO
T6:NO
T7:YES
T8:NO
T9:YES
T10:NO
T11:YES
T12:YES
T13:NO
T14:NO
$_num = "1": <=1
$_num = "2": ...
$_num = "3": ...
$_num = "4": >=4
$_num = "5": >=4
$_num = "6": >=4
$_num = "7": >=4
$_num = "8": >=4
$_num = "9": >=4
Item 6 by condition $_num > 5
Item 7 by condition $_num > 5
Item 8 by condition $_num > 5
Item 9 by condition $_num > 5
Item 10 by condition $_num > 5
