--TEST--
literal #2
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ LITERAL }}
{{ \$var }}
{{ END }}{{ END }}

{{ BEGIN x }}* this is x; {{ if(\$c, "c is not empty!"); }} *{{ END }}
{{ BEGIN y }}* this is y; b={{ \$b }} *{{ END }}
{{ BEGIN z }}* this is z *{{ END }}

{{ LITERAL }}{{ BEGIN }}
{{ BEGIN }}
{{ END }}{{ END }}
BODY;

$T = new Blitz();
$T->load($body);

$params = array(
    'a' => 'a_value',
    'b' => 'b_value',
    'c' => 'c_value'
);

$blocks = array('x','y','z');

foreach($blocks as $i_block) {
    echo $T->fetch($i_block, $params)."\n";
}

?>
--EXPECT--
end with no begin (noname_loaded_from_zval: line 3, pos 10)
end with no begin (noname_loaded_from_zval: line 11, pos 10)
* this is x; c is not empty! *
* this is y; b=b_value *
* this is z *
