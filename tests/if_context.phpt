--TEST--
conditional contexts (multi-line conditions): if/end and unless/end
--FILE--
<?php

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
{{ IF global_var }}
global var is set
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);

$T->setGlobal(array('global_var' => 1));

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

unset($T);
$body = '
path-variables test:
{{ IF $user.login }}
    IF: object is OK
{{ END }}
{{ IF $hash.sub.key }}
    IF: hash is OK
{{ END }}
{{ UNLESS $user.dummy }}
    UNLESS: object is OK
{{ END }}
{{ UNLESS $hash.dummy.dummy }}
    UNLESS: hash is OK
{{ END }}
';

class User {
    var $login;
    function User() {
        $this->login = TRUE;
    }
}

$T = new Blitz();
$T->load($body);

echo $T->parse(
    array(
        0 => array(
            'user' => new User(),
            'hash' => array('sub' => array('key' => 1))
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
global var is set

path-variables test:
    IF: object is OK
    IF: hash is OK
    UNLESS: object is OK
    UNLESS: hash is OK
