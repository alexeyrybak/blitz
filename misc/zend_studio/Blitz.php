<?php
  // do not include this file from your scripts
  // simply add it to your Zend Studio project
  
  if (!extension_loaded('Blitz')) {
    
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
    }
    
  }