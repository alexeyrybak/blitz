--TEST--
magic vars issues
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$T = new Blitz();
$tmp = '{{BEGIN item}}
  {{id}} - {{title}}
  $_ is: {{ var_export($_, TRUE) }}
  $_top is: {{ var_export($_top, TRUE) }}
  {{BEGIN child}}
    {{id}} - {{title}}
    $_ is: {{ var_export($_, true) }}
    $_top is: {{ var_export($_top, true) }}
    $_parent: {{ var_export($_parent, true) }}
  {{END}}
{{END}}';
$T->load($tmp);
$T->display(
    array(
        'item' => array(
            'id' => 1,
            'title' => 'Parent',
            'child' => array(
                'id' => 1,
                'title' => 'Child'
            )
        )
    )
);

?>
--EXPECT--
1 - Parent
  $_ is: array (
  'id' => 1,
  'title' => 'Parent',
  'child' => 
  array (
    'id' => 1,
    'title' => 'Child',
  ),
)
  $_top is: array (
  'item' => 
  array (
    'id' => 1,
    'title' => 'Parent',
    'child' => 
    array (
      'id' => 1,
      'title' => 'Child',
    ),
  ),
)
    1 - Child
    $_ is: array (
  'id' => 1,
  'title' => 'Child',
)
    $_top is: array (
  'item' => 
  array (
    'id' => 1,
    'title' => 'Parent',
    'child' => 
    array (
      'id' => 1,
      'title' => 'Child',
    ),
  ),
)
    $_parent: array (
  'id' => 1,
  'title' => 'Parent',
  'child' => 
  array (
    'id' => 1,
    'title' => 'Child',
  ),
)
