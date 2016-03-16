/* Copyright (c) 2015 Digiverse d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. The
 * license should be included in the source distribution of the Software;
 * if not, you may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * The above copyright notice and licensing terms shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/XmlOutputter.h>

namespace cu = CppUnit;

int main(int argc, char **argv)
{
  cu::TestResult testresult;
  
  // register listener for collecting the test-results
  cu::TestResultCollector collectedresults;
  testresult.addListener (&collectedresults);

  // register listener for per-test progress output
  cu::BriefTestProgressListener progress;
  testresult.addListener (&progress);


  cu::TestRunner runner;
  cu::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
  runner.addTest(registry.makeTest());
  runner.run(testresult);

  // output results in compiler-format
//  cu::CompilerOutputter compileroutputter(&collectedresults, std::cerr);
//  compileroutputter.write ();

  cu::TextOutputter outputter(&collectedresults, std::cerr);
  outputter.write ();
  
//  outputter.printHeader();
//  outputter.printStatistics();
//  outputter.printFailures();
  // Output XML for Jenkins CPPunit plugin
//  std::ofstream xmlFileOut("cppTestBasicMathResults.xml");
//  cu::XmlOutputter xmlOut(&collectedresults, xmlFileOut);
//  xmlOut.write();

  return !collectedresults.wasSuccessful();
}