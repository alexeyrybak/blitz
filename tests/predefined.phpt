--TEST--
predefined loop variables: $_total, $_num, $_even, $_odd, $_first, $_last
--FILE--
<?php
include('common.inc');

$body = 'predefined loop variables
{{ BEGIN root }} root: {{ $i }}, total: {{ $_total }}, num: {{ $_num }}, even: {{ $_even }}, {{ $_odd }}; attrs: {{ if($_even,"+even"); }}{{ if($_odd,"+odd"); }}{{ if($_first,"+first"); }}{{ if($_last,"+last"); }}
{{ BEGIN child }} ==== child: {{ $j }}, total: {{ $_total }}, num: {{ $_num }}, even: {{ $_even }}, {{ $_odd }}; attrs: {{ if($_even,"+even"); }}{{ if($_odd,"+odd"); }}{{ if($_first,"+first"); }}{{ if($_last,"+last"); }}
{{ END }}
{{ END }}';

$T = new Blitz();
$T->load($body);

$data = array(
    'root' => array(
        0 => array('i' => 1),
        1 => array('i' => 2),
        2 => array(
            'i' => 3,
            'child' => array(
                0 => array('j' => 1),
                1 => array('j' => 2),
                2 => array('j' => 3)
            )
        ),
        3 => array(
            'i' => 4,
            'child' => array(
                0 => array('j' => 1),
                1 => array('j' => 2),
                2 => array('j' => 3),
                3 => array('j' => 4),
                4 => array('j' => 5),
                5 => array('j' => 6),
                6 => array('j' => 7),
                7 => array('j' => 8),
                8 => array('j' => 9),
                9 => array('j' => 10),
            )
        ),
    )
);

$T->set($data);

echo $T->parse();

?>
--EXPECT--
predefined loop variables
 root: 1, total: 4, num: 1, even: 1, 0; attrs: +even+first

 root: 2, total: 4, num: 2, even: 0, 1; attrs: +odd

 root: 3, total: 4, num: 3, even: 1, 0; attrs: +even
 ==== child: 1, total: 3, num: 1, even: 1, 0; attrs: +even+first
 ==== child: 2, total: 3, num: 2, even: 0, 1; attrs: +odd
 ==== child: 3, total: 3, num: 3, even: 1, 0; attrs: +even+last

 root: 4, total: 4, num: 4, even: 0, 1; attrs: +odd+last
 ==== child: 1, total: 10, num: 1, even: 1, 0; attrs: +even+first
 ==== child: 2, total: 10, num: 2, even: 0, 1; attrs: +odd
 ==== child: 3, total: 10, num: 3, even: 1, 0; attrs: +even
 ==== child: 4, total: 10, num: 4, even: 0, 1; attrs: +odd
 ==== child: 5, total: 10, num: 5, even: 1, 0; attrs: +even
 ==== child: 6, total: 10, num: 6, even: 0, 1; attrs: +odd
 ==== child: 7, total: 10, num: 7, even: 1, 0; attrs: +even
 ==== child: 8, total: 10, num: 8, even: 0, 1; attrs: +odd
 ==== child: 9, total: 10, num: 9, even: 1, 0; attrs: +even
 ==== child: 10, total: 10, num: 10, even: 0, 1; attrs: +odd+last

