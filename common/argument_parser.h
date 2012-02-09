/*
Copyright (c) 2011 TU Dresden - Database Technology Group

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>

Current version: 1.0 (released December 14, 2011)

Version history:
  - 1.0 Initial release (December 14, 2011) 
*/

/** @file
A simple command line parser.

Defines the ArgumentParser class that helps writing user-friendly commandline interfaces.
It is inspired by Python's argparse module (http://docs.python.org/library/argparse.html).
*/

#ifndef _COMMON_ARGUMENT_PARSER_H_
#define _COMMON_ARGUMENT_PARSER_H_

#include <list>
#include <map>
#include <string>
#include <stdio.h>
#include <vector>

#include <common/macros.h>

class Argument;
class Value;

// A class defining a simple command line argument parser.
//
// Usage:
//
//   int main(int argc, char** argv){
//     ArgumentParser parser;
//     parser.add_argument("-t","--test").nargs(1).metavar("<value>").help("A sample argument including a value");
//     
//     parser.parse_args(argc,argv);
//     
//     if(parser.is_set("-t")){
//       int value = parser.get_value("-t").toInt();
//       // do something with the value...
//     }
//   }
class ArgumentParser{
 public:
  // Constructor for ArgumentParser
  ArgumentParser();

  // Destructor for ArgumentParser
  ~ArgumentParser();
    
  // Setter functions
  ArgumentParser& description(std::string description){ description_=description; return *this;};
  ArgumentParser& prolog(std::string prolog){ prolog_=prolog; return *this;};
  ArgumentParser& epilog(std::string epilog){ epilog_=epilog; return *this;};
  ArgumentParser& add_help(bool add_help){ add_help_=add_help; return *this;};
  ArgumentParser& prog(std::string prog){ prog_=prog;return *this;};
  ArgumentParser& usage(std::string usage){ usage_=usage;return *this;};
  ArgumentParser& indent(std::string indent){ indent_=indent; return *this;};
  ArgumentParser& padding(unsigned int padding){ padding_=padding; return *this;}

  // Getter functions
  const std::string& description() const {return description_;};
  const std::string& prolog() const {return prolog_;};
  const std::string& epilog() const {return epilog_;};
  bool add_help() const {return add_help_;};
  const std::string& prog() const {return prog_;};
  const std::string& usage() const {return usage_;};
  const std::string& indent() const {return indent_;};
  unsigned int padding() const {return padding_;};
    
  // Add an argument
  Argument& add_argument(std::string name);
  Argument& add_argument(std::string name, std::string alternative_name);
  Argument& add_argument(std::list<std::string> names);
        
  // Parse the parameters
  bool parse_args(int argc, char** argv);

  // Print the help to the standard output
  void print_help();

  // Returns if the argument given by its name has been set.
  // If it is called before parse_args() it will return false.
  bool is_set(std::string name);
    
  // Returns the value of an argument (given by its name).
  // If the given argument contains more than one value, index can be used
  // to specify which value will be returned.
  Value* get_value(std::string name, unsigned int index = 0);
    
  // Generates a string describing the usage of the program
  std::string usage_string();
    
  // Generates a string describing the given argument
  std::string option_string(std::string argument);
    
  // Generates the help string
  std::string help_string();
    
  // Returns a pointer to the argument, specified by its name - or null.
  Argument* get_argument(std::string name);
  
 private:
  // List of arguments
  std::map<std::string, Argument*> arguments_;

  // A map containg all alternative names to a specified argument
  std::map<std::string,std::list<std::string> > alternative_args_;

  // An ordered list of all argument names
  std::list<std::string> arg_list_;

  // String that is used to indent content inside the help string
  std::string indent_;
  
  // The width that the argument string will be padded to (in the option string)
  unsigned int padding_;

  // Text to display between usage string and argument help
  std::string prolog_;

  // Text to display before the argument help.
  std::string description_;

  // Text to display after the argument help.
  std::string epilog_;

  // Whether to add a help option (-h and --help) option (default value is true)
  bool add_help_;

  // The name of the program (default is argv[0])
  std::string prog_;

  // The string describing the program usage (default will be generated automatically)
  std::string usage_;

  DISALLOW_COPY_AND_ASSIGN(ArgumentParser);
};


// Defines a command line argument including its values.
//
// Sample usage:
//   Argument argument("--help");
//   argument.nargs(0).help("Displays this message")
class Argument{
 public:
    // Constructor for Argument
    Argument();

    // Destructor for Argument
    ~Argument();

    // Setter functions
    Argument& nargs(unsigned int nargs){nargs_=nargs;return *this;};
    Argument& default_value(std::string default_value){default_value_=default_value;return *this;};
    Argument& choices(std::list<std::string> choices){choices_=choices; return *this; };
    Argument& required(bool required){ required_=required; return *this; };
    Argument& help(std::string help){ help_=help; return *this; };
    Argument& metavar(std::string metavar){ metavar_=metavar; return *this; };
    Argument& set(bool set){ set_=set; return *this; };
    Argument& add_value(std::string value);

    // Getter functions
    unsigned int nargs() const { return nargs_; };
    const std::string& default_value() const { return default_value_; };
    std::list<std::string> choices() const{ return choices_; };
    bool required() const { return required_; };
    const std::string& help() const { return help_; };
    const std::string& metavar() const {return metavar_; };
    bool set() const { return set_; };
    Value& value(unsigned int index = 0) const { Value * val = values_[index];return (*val);};

  private:
    // The name of this argument
    std::string name_;

    // The number of arguments that will be consumed. (Default value is 0.)
    unsigned int nargs_;

    // The default value that will be returned if the argument is absent from
    // the command line.
    std::string default_value_;
    
    // A list of the allowed values for the argument.
    std::list<std::string> choices_;

    // Wheather or not the argument is required. (Default value is false.)
    bool required_;

    // A brief description of what the argument does.
    std::string help_;

    // A name for the arguments in usage and help messages.
    std::string metavar_;

    // Whether or not the argument is set.
    bool set_;

    // The values of this argument.
    std::vector<Value*> values_;

    DISALLOW_COPY_AND_ASSIGN(Argument);
};


// Defines a value of a command line argument.
//
// This class can be used to securely convert a string into other data types
// like int or bool.
//
// Sample usage:
//   Value value(0);
//   int int_value = value.toInt();
//   bool bool_value = value.toBool();
class Value{
  public:
    // Constructor for Value
    explicit Value(std::string value);

    // Destructor for Value
    ~Value();

    // Set the value
    Value& set(std::string value){ value_=value; return *this;};
    
    // Return the value as a string
    const std::string& get(){ return value_; };
    
    // Return the value as int
    int toInt();

    // Return the value as bool
    bool toBool();

    // Return the value as float
    float toFloat();

    // Return the value as double
    double toDouble();
    
    // Stream operators
    friend std::ostream& operator<<(std::ostream &stream, const Value& value);
    friend std::istream& operator>>(std::istream &stream, Value& value);

  private:
    // The value as a string
    std::string value_;

    DISALLOW_COPY_AND_ASSIGN(Value);
};

#endif // _COMMON_ARGUMENT_PARSER_H_
