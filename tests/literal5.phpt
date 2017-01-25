--TEST--
literal #5
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ LITERAL }}+=test=+{{ LITERAL }}+{{ function some() { return "{{" } }}*{{ END }}
{{ LITERAL }}
<h2>To-do list</h2>

<input on-change='newTodo' on-enter='blur' placeholder='What needs to be done?'>

<ul class='todos'>
  {{#each items:i}}
    <li class='{{ done ? "done" : "pending" }}'>
      <input type='checkbox' checked='{{done}}'>
      <span class='description' on-tap='edit'>
        {{description}}

        {{#if editing}}
          <input class='edit'
                 value='{{description}}'
                 on-blur='stop_editing'>
        {{/if}}
      </span>
      <span class='button' on-tap='remove'>x</span>
    </li>
  {{/each}}
</ul>
{{ END }}
BODY;


$T = new Blitz();
$T->load($body);
$T->display();

?>
--EXPECTF--
+=test=+{{ LITERAL }}+{{ function some() { return "{{" } }}*
<h2>To-do list</h2>

<input on-change='newTodo' on-enter='blur' placeholder='What needs to be done?'>

<ul class='todos'>
  {{#each items:i}}
    <li class='{{ done ? "done" : "pending" }}'>
      <input type='checkbox' checked='{{done}}'>
      <span class='description' on-tap='edit'>
        {{description}}

        {{#if editing}}
          <input class='edit'
                 value='{{description}}'
                 on-blur='stop_editing'>
        {{/if}}
      </span>
      <span class='button' on-tap='remove'>x</span>
    </li>
  {{/each}}
</ul>
