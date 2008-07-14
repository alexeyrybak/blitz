--TEST--
conditional contexts (multi-line conditions): if/end and unless/end
--FILE--
<?

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ BEGIN some }}
some began
{{ IF bla }}
<if>
{{ BEGIN bla }}
item = {{ item }}
{{ END }}
</if>
{{ END }}
{{ UNLESS dummy }}
unless worked: {{ var }}
{{ END }}
some ended
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);

echo $T->parse(
    array(
        'some' => array(
            0 => array(
                'var' => 'sure',
                'bla' => array(
                    0 => array(
                        'item' => 'hello, world!',
                    ), 
                    1 => array(
                        'item' => 'hello, office zombies!',
                    )
                ) 
            )
        )
    )
);
?>
--EXPECT--
some began
<if>
item = hello, world!
item = hello, office zombies!
</if>
unless worked: sure
some ended
