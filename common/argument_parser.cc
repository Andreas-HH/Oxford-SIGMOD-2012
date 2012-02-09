/*
Copyright (c) 2011 TU Dresden - Database Technology Group

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>

Current version: 1.0 (released December 14, 2011)

Version history:
  - 1.0 Initial release (December 14, 2011) 
*/

/** @file
A simple command line parser.

@see common/argument_parser.h
*/

#include <iomanip>
#include <iostream>
#include <sstream>

#include "argument_parser.h"

/**
Constructor for ArgumentParser.

Creates a new ArgumentParser and performs all necessary initializations.
*/
ArgumentParser::ArgumentParser(){
  this->prog_ = "";
  this->add_help_ = true;
  this->indent("\t");
  this->padding(50);
};

/**
Destructor for ArgumentParser.

Performs all necessary clean-up work.
*/
ArgumentParser::~ArgumentParser(){
  // nothing...
};

/**
Add an available argument to the ArgumentParser.

@param name
  the argument as a string

@return
  the newly created argument
*/
Argument& ArgumentParser::add_argument(std::string name){
  std::list<std::string> names;
  names.push_back(name);
  return this->add_argument(names);
};

/**
Add an available argument including an alternative name to the ArgumentParser.

@param name
  the argument as a string

@param alternative_name
  the alternative argument name as a string

@return
  the newly created argument
*/
Argument& ArgumentParser::add_argument(std::string name, std::string alternative_name){
  std::list<std::string> names;
  names.push_back(name);
  names.push_back(alternative_name);
  return this->add_argument(names);
};

/**
Add an available argument and a list of alternative names to the ArgumentParser.

@param names
  a list of equivalent names to reference the new argument

@return
  the newly created argument
*/
Argument& ArgumentParser::add_argument(std::list<std::string> names){
  Argument* argument = new Argument();
  std::string& s = names.front();
  arg_list_.push_back(s) ;
  arguments_[ s ] = argument;
  names.pop_front();

  while (!names.empty()){
    alternative_args_[s].push_back(names.front());
    arguments_[names.front()] = argument;
    names.pop_front();
  }
  
  return (*argument);
};

/**
Parse the command line arguments.

@param argc
  the number of passed arguments

@param argv
  a vector of strings

@return
  true if the passed arguments could be parsed without errors
  (and without occurrence of the help argument), otherwise false
*/
bool ArgumentParser::parse_args(int argc, char** argv){
  if(!this->prog().empty())
    this->prog(argv[0]);

  // Add the help option
  if(this->add_help())
    this->add_argument("-h","--help").help("Display this message");

  for(int i = 1; i < argc; i++){
    char* arg = argv[i];
    Argument* argument = this->get_argument(arg);
    if(argument != NULL){
      argument->set(true);
      for(unsigned int j = 0; j < argument->nargs(); j++){
        i++;
        if(i < argc){
          argument->add_value(argv[i]);
        } else {
          std::cerr << "Option '" << arg << "' requires " << argument->nargs()
                    << ((argument->nargs()==1)?" argument.":" arguments.") << std::endl;
          return false;
        }
      }
    } else {
      std::cerr << "Unknown option '" << arg << "'." << std::endl;
      return false;

    }
  }

  if(this->is_set("-h") && add_help()){
    this->print_help();
    return false;
  }

  return true;
};

/**
Print the help string to the standard output
*/
void ArgumentParser::print_help(){
  std::cout << this->help_string() << std::endl;
}

/**
Returns if the argument given by its name has been set.

If it is called before parse_args() it will return false.

@param name
  the name of the argument to check

@returns
  true if the argument has been set, otherwise false
*/
bool ArgumentParser::is_set(std::string name){
  Argument* argument = this->get_argument(name);
  if(argument == NULL)
    return false;
  return argument->set();   
};

/**
Returns the value of an argument (given by its name).
 
If the given argument contains more than one value, index can be used
to specify which value will be returned.

@param name
  the name of the argument

@param index
  the index of the values (default is 0)

@return
  A pointer to the corresponding value or NULL (if not existent)
*/
Value* ArgumentParser::get_value(std::string name, unsigned int index){
  Argument* argument = this->get_argument(name);
  if(argument != NULL){
    return &argument->value(index);
  }
  return NULL;
}

/**
Generates a string describing the usage of the program.

If a usage string has been explicitly defined it will be returned,
otherwise it will be generated using the program name and the provided
arguments.

@return
  a string describing how to use the program
*/
std::string ArgumentParser::usage_string(){
  std::stringstream result;

  if(!this->usage().empty())
    return this->usage();

  if(!this->prog().empty())
     result << this->prog();
  
  std::list<std::string>::iterator it;
  for ( it = arg_list_.begin() ; it != arg_list_.end(); it++ ){
    std::map<std::string, Argument*>::const_iterator itt = arguments_.find(*it);
    
    if (itt != arguments_.end()){
      Argument* p = itt->second;
      if(p != NULL){
        result << " ";
        if(!p->required())
          result << "[";
        
        result << (*it);
        if(!p->metavar().empty())
          result << " " << p->metavar();        

        if(!p->required())
          result << "]";
      }
    }
  }
  return result.str();
};

/**
Generates a string describing the given argument.

The option string is generated using all possible argument names
(including there meta-string) and its description.

@param argument
  the name of the argument

@returns
  the option string for the given argument
*/
std::string ArgumentParser::option_string(std::string argument){
  std::stringstream result;
  
   Argument* arg = this->get_argument(argument);
   if(arg != NULL){
      result << argument;
      if(!arg->metavar().empty())
        result << " " << arg->metavar();
      
      std::list<std::string>::iterator it;
      for(it = alternative_args_[argument].begin(); it != alternative_args_[argument].end(); it++){
        result << ", " << (*it);
        if(!arg->metavar().empty())
          result << " " << arg->metavar();
      }
      
      // Pad the string with whitespaces on the right
      std::string padding(this->padding()-result.str().length(), ' ');
      result << padding << arg->help();
  }
  return result.str();
};

/**
Generates a help string.

The help string consists of a general description and a usage line, as well as a list of
arguments and there corresponding description.

If defined, the "Options:" section can be enclosed by a prolog and an epilog.

@return
  the generated help string
*/
std::string ArgumentParser::help_string(){
  std::stringstream result;
  // Add the description
  if(!this->description().empty())
    result << this->description() << std::endl << std::endl;
  
  // Add a usage line
  result << "Usage:" << std::endl << this->indent() << this->usage_string() << std::endl;
  
  // Add the prolog
  if(!this->prolog().empty())
    result << this->prolog() << std::endl;

  // Add the options
  result << std::endl << "Options:" << std::endl;
  std::list<std::string>::iterator it;
  for ( it = arg_list_.begin() ; it != arg_list_.end(); it++ ){
     result << this->indent() << this->option_string(*it) << std::endl;
  };
  
  // Add the epilog
  if(!this->epilog().empty())
    result << this->epilog() << std::endl;

  return result.str();
};

/**
Returns a pointer to an argument, specified by its name.

@param name
  the name of the argument to retrieve

@return
  A pointer to the argument with the given name 
  or NULL if no matching argument has been found.
*/
Argument* ArgumentParser::get_argument(std::string name){
  std::map<std::string, Argument*>::const_iterator it = arguments_.find(name);
  if (it == arguments_.end()){
    return NULL;
  }else{
    return it->second;
  }
};

/**
Constructor for Argument.

Creates a new Argument and performs all necessary initializations.
*/
Argument::Argument(){
  this->required(false);
  this->metavar("");
  this->set(false);
  this->nargs(0);
  this->help("test");
};

/**
Destructor for Argument.

Performs all necessary clean-up work.
*/
Argument::~Argument(){
  // nothing...
};

/**
Add a value to the argument

@param value
  the value as a string

@return
  the current argument itself
*/
Argument& Argument::add_value(std::string value){
  Value *val = new Value(value);
  values_.push_back(val);
  return *this;
};

/**
Constructor for Value.

Creates a new Value from the given string.

@param value
  the value as a string
*/
Value::Value(std::string value){
  value_ = value;
};

/**
Destructor for Value.

Performs all necessary clean-up work.
*/
Value::~Value(){
  // nothing...
};

/**
Return the value as an integer
*/
int Value::toInt(){
  int result;
  std::stringstream stream(value_); 
  stream >> result; 
  return result; 
};

/**
Return the value as a Boolean variable.

@return
  false if the value is "false" or "0", otherwise true
*/
bool Value::toBool(){
  if( (value_.compare("false") == 0) || (value_.compare("0") == 0))
    return false;

  return true;
};

/**
Return the value as a variable of type float.
*/
float Value::toFloat(){
   float result;
   std::stringstream stream(value_);
   stream >> result;
   return result;
};

/**
Return the value as a variable of type double.
*/
double Value::toDouble(){
   double result;
   std::stringstream stream(value_);
   stream >> result;
   return result;
};

/**
Prints the value using the C++ I/O stream API.
*/
std::ostream& operator<<(std::ostream &stream, const Value& value){
  stream << value.value_;
  return stream;
};

/**
Writes to the value from a stream.
*/
std::istream& operator>>(std::istream &stream, Value& value){
  stream >> value.value_;
  return stream;
};
