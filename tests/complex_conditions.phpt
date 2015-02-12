--TEST--
complex conditions
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
Complex conditions
{{IF ((a == "1" || a == "2") && b == "3")}}
  A is one or twoo WHILE b is three
{{ELSE}}
  A is not one or twoo OR b is not three
{{END}}

{{ IF sortOrder != 'PRICE_DESC' }}ASC{{ ELSE }}DESC{{ END }}
{{ UNLESS sortOrder == 'PRICE_DESC' }}ASC{{ ELSE }}DESC{{ END }}

BODY;

$T = new Blitz();
$T->load($body);
$T->display(
    array(
        'a' => 2,
        'b' => 3,
        'c' => false,
        'sortOrder' => 'PRICE_DESC'
    )
);

?>
--EXPECT--
Complex conditions
  A is one or twoo WHILE b is three

DESC
DESC
