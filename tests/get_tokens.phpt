--TEST--
get template tokens
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);
ini_set('blitz.warn_context_duplicates', 0);

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
    {{ IF bla }} bla again {{ END }}
    {{ UNLESS dummy }}
    unless worked: {{ var }}
    {{ END }}
    {{escape('some ended')}}
    {{escape(\$a, 1, false)}}
    {{escape(CONSTANT_DEFINED)}}
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);
echo json_encode($T->getTokens(),JSON_PRETTY_PRINT);
?>
--EXPECT--
[
    {
        "lexem": "BEGIN",
        "type": 26,
        "begin": 0,
        "begin_shift": 17,
        "end": 365,
        "end_shift": 356,
        "args": [
            {
                "name": "some",
                "type": 0
            }
        ],
        "children": [
            {
                "lexem": "IF",
                "type": 94,
                "begin": 32,
                "begin_shift": 49,
                "end": 162,
                "end_shift": 148,
                "args": [
                    {
                        "name": "bla",
                        "type": 1
                    }
                ],
                "children": [
                    {
                        "lexem": "BEGIN",
                        "type": 26,
                        "begin": 62,
                        "begin_shift": 86,
                        "end": 134,
                        "end_shift": 116,
                        "args": [
                            {
                                "name": "bla",
                                "type": 0
                            }
                        ],
                        "children": [
                            {
                                "lexem": "item",
                                "type": 5,
                                "begin": 105,
                                "begin_shift": 0,
                                "end": 115,
                                "end_shift": 0
                            },
                            {
                                "lexem": "END",
                                "type": 22,
                                "begin": 116,
                                "begin_shift": 134,
                                "end": 133,
                                "end_shift": 133
                            }
                        ]
                    },
                    {
                        "lexem": "END",
                        "type": 22,
                        "begin": 148,
                        "begin_shift": 162,
                        "end": 161,
                        "end_shift": 161
                    }
                ]
            },
            {
                "lexem": "IF",
                "type": 94,
                "begin": 166,
                "begin_shift": 178,
                "end": 198,
                "end_shift": 189,
                "args": [
                    {
                        "name": "bla",
                        "type": 1
                    }
                ],
                "children": [
                    {
                        "lexem": "END",
                        "type": 22,
                        "begin": 189,
                        "begin_shift": 198,
                        "end": 198,
                        "end_shift": 198
                    }
                ]
            },
            {
                "lexem": "UNLESS",
                "type": 98,
                "begin": 199,
                "begin_shift": 222,
                "end": 265,
                "end_shift": 251,
                "args": [
                    {
                        "name": "dummy",
                        "type": 1
                    }
                ],
                "children": [
                    {
                        "lexem": "var",
                        "type": 5,
                        "begin": 241,
                        "begin_shift": 0,
                        "end": 250,
                        "end_shift": 0
                    },
                    {
                        "lexem": "END",
                        "type": 22,
                        "begin": 251,
                        "begin_shift": 265,
                        "end": 264,
                        "end_shift": 264
                    }
                ]
            },
            {
                "lexem": "escape",
                "type": 46,
                "begin": 269,
                "begin_shift": 0,
                "end": 293,
                "end_shift": 0,
                "args": [
                    {
                        "name": "some ended",
                        "type": 4
                    }
                ]
            },
            {
                "lexem": "escape",
                "type": 46,
                "begin": 298,
                "begin_shift": 0,
                "end": 322,
                "end_shift": 0,
                "args": [
                    {
                        "name": "a",
                        "type": 1
                    },
                    {
                        "name": "1",
                        "type": 8
                    },
                    {
                        "name": "",
                        "type": 16
                    }
                ]
            },
            {
                "lexem": "escape",
                "type": 46,
                "begin": 327,
                "begin_shift": 0,
                "end": 355,
                "end_shift": 0,
                "args": [
                    {
                        "name": "CONSTANT_DEFINED",
                        "type": 1
                    }
                ]
            },
            {
                "lexem": "END",
                "type": 22,
                "begin": 356,
                "begin_shift": 365,
                "end": 365,
                "end_shift": 365
            }
        ]
    }
]
