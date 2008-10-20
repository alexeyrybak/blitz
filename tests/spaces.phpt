--TEST--
remove empty spaces around context tags
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
  {{ BEGIN a }}  
    hello, a
    {{ BEGIN b }}    
        hello, b
        {{ IF c }}
            hello, c
        {{ END }}
    {{ END }}
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);
echo "=====\n";
echo $T->parse(
    array(
        'a' => array(array(),
            array(
                'b' => array(
                    array('c' => TRUE),
                    array('c' => FALSE),
                )
            ),
            array(
                'b' => array(
                    array('c' => 0),
                )
            ),
        )
    )
);
echo "=====\n";
?>
--EXPECT--
=====
    hello, a
    hello, a
        hello, b
            hello, c
        hello, b
    hello, a
        hello, b
=====
