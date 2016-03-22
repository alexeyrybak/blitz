<?php
  // do not include this file from your scripts
  // simply add it to your Zend Studio project
  
  if (!extension_loaded('Blitz')) {

    define('BLITZ_TYPE_VAR', 1);
    define('BLITZ_TYPE_METHOD', 2);

    define('BLITZ_ARG_TYPE_VAR', 1);
    define('BLITZ_ARG_TYPE_VAR_PATH', 2);
    define('BLITZ_ARG_TYPE_STR', 4);
    define('BLITZ_ARG_TYPE_NUM', 8);
    define('BLITZ_ARG_TYPE_BOOL', 16);
    define('BLITZ_ARG_TYPE_FLOAT', 32);
    define('BLITZ_ARG_TYPE_EXPR_SHIFT', 128);

    define('BLITZ_EXPR_OPERATOR_GE', 128);
    define('BLITZ_EXPR_OPERATOR_G', 129);
    define('BLITZ_EXPR_OPERATOR_LE', 130);
    define('BLITZ_EXPR_OPERATOR_L', 131);
    define('BLITZ_EXPR_OPERATOR_NE', 132);
    define('BLITZ_EXPR_OPERATOR_E', 133);
    define('BLITZ_EXPR_OPERATOR_LA', 134);
    define('BLITZ_EXPR_OPERATOR_LO', 135);
    define('BLITZ_EXPR_OPERATOR_N', 136);
    define('BLITZ_EXPR_OPERATOR_LP', 137);
    define('BLITZ_EXPR_OPERATOR_RP', 138);

    define('BLITZ_NODE_TYPE_COMMENT', 0);
    define('BLITZ_NODE_TYPE_IF', 6);
    define('BLITZ_NODE_TYPE_UNLESS', 10);
    define('BLITZ_NODE_TYPE_INCLUDE', 14);
    define('BLITZ_NODE_TYPE_END', 22);
    define('BLITZ_NODE_TYPE_CONTEXT', 26);
    define('BLITZ_NODE_TYPE_CONDITION', 30);

    define('BLITZ_NODE_TYPE_WRAPPER_ESCAPE', 46);
    define('BLITZ_NODE_TYPE_WRAPPER_DATE', 50);
    define('BLITZ_NODE_TYPE_WRAPPER_UPPER', 54);
    define('BLITZ_NODE_TYPE_WRAPPER_LOWER', 58);
    define('BLITZ_NODE_TYPE_WRAPPER_TRIM', 62);

    define('BLITZ_NODE_TYPE_IF_CONTEXT', 94);
    define('BLITZ_NODE_TYPE_UNLESS_CONTEXT', 98);
    define('BLITZ_NODE_TYPE_ELSEIF_CONTEXT', 106);
    define('BLITZ_NODE_TYPE_ELSE_CONTEXT', 114);

    define('BLITZ_NODE_TYPE_VAR', 5);
    define('BLITZ_NODE_TYPE_VAR_PATH', 9); 
    
    class Blitz 
    {
      /**
       * Set context and iterate it
       *
       * @since  (blitz >= 0.4)
       * @param  string $context_path
       * @param  array  $parameters
       * @return bool
       */
      public function block($context_path, $parameters = array()) 
      {}
      
      /**
       * Clean up context variables and iterations
       *
       * @since  (blitz >= 0.4.10)
       * @param  string $context_path
       * @param  bool $warn_notfound
       * @return bool
       */
      public function clean($context_path='/', $warn_notfound = true)
      {}
      
      /**
       * Set current context
       *
       * @since  (blitz >= 0.4)
       * @param  string $context_path
       * @return bool
       */
      public function context($context_path)
      {}
      
      /**
       * Dump iteration, debug method
       *
       * @since  (blitz >= 0.4)
       * @return bool
       */
      public function dumpIterations()
      {}
      
      /**
       * Dump iteration, debug method
       *
       * @since  (blitz >= 0.4)
       * @return bool
       */
      public function dump_iterations()
      {}      
      
      /**
       * Dump template structure
       *
       * @since  (blitz >= 0.4)
       * @return bool
       */
      public function dumpStruct()
      {}
      
      /**
       * Dump template structure
       *
       * @since  (blitz >= 0.4)
       * @return bool
       */
      public function dump_struct()
      {}

      /**
       * Fetch the result of context
       *
       * @since  (blitz >= 0.4)
       * @param  string $context_path
       * @param  array  $parameters
       * @return string
       */
      public function block($context_path, $parameters = array()) 
      {}

      /**
       * Check if context exists
       *
       * @since  (blitz >= 0.4)
       * @param  string $context_path
       * @return bool
       */
      public function hasContext($context_path)
      {}   
      
      /**
       * Check if context exists
       *
       * @since  (blitz >= 0.4)
       * @param  string $context_path
       * @return bool
       */
      public function has_context($context_path)
      {}        
      
      /**
       * Include another template (method Blitz::include()).
       * 
       * Using underscore 'cause include is reserved php word
       *
       * @since  (blitz >= 0.1)
       * @param  string $template_name
       * @param  array  $global_vars
       * @return string
       */
      public function _include($template_name, $global_vars=array()) // include
      {}
      
      /**
       * Iterate context
       *
       * @since  (blitz >= 0.4)
       * @param  string $context_path
       * @return bool
       */
      public function iterate($context_path = '/')
      {}
      
      /**
       * Load template body from PHP string variable
       *
       * @since  (blitz >= 0.4)
       * @param  string $tpl
       * @return bool
       */
      public function load($tpl)
      {}      
      
      /**
       * Build and return the result
       *
       * @since  (blitz >= 0.1)
       * @param  array $global_vars
       * @return string
       */
      public function parse($global_vars = array())
      {}
      
      /**
       * Set variables or iterations
       *
       * @since  (blitz >= 0.1)
       * @param  array $parameters
       * @return bool
       */
      public function set($parameters)
      {}
      
      /**
       * Set global variables
       *
       * @since  (blitz >= 0.4)
       * @param  array $parameters
       * @return bool
       */
      public function setGlobal($parameters)
      {}

      /**
       * Returns template path list.
       *
       * @return array
       */
      public function getStructure()
      {}

      /**
       * Returns tokenized template structure.
       * Just like dumpStruct but returns structure array.
       *
       * @return array
       */
      public function getTokens()
      {}

      /**
       * Changes autoescape policy for current class.
       * Returns old policy value.
       *
       * @param bool $value
       *
       * @return bool
       */
      public function setAutoEscape($value)
      {}

      /**
       * Returns autoescape policy for current class.
       *
       * @return bool
       */
      public function getAutoEscape()
      {}
    }
    
  }
