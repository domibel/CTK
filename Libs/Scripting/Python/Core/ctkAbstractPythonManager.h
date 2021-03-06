/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

#ifndef __ctkAbstractPythonManager_h
#define __ctkAbstractPythonManager_h

// Qt includes
#include <QObject>
#include <QList>
#include <QStringList>

// CTK includes
#include "ctkScriptingPythonCoreExport.h"

class ctkAbstractPythonManagerPrivate;
class PythonQtObjectPtr;

/// \ingroup Scripting_Python_Core
class CTK_SCRIPTING_PYTHON_CORE_EXPORT ctkAbstractPythonManager : public QObject
{
  Q_OBJECT

public:
  typedef QObject Superclass;
  ctkAbstractPythonManager(QObject* _parent=NULL);
  virtual ~ctkAbstractPythonManager();

  /// Calling this function after mainContext() has been called at least once is a no-op.
  /// If not overridden calling this function, the default initialization flags are
  /// PythonQt::IgnoreSiteModule and PythonQt::RedirectStdOut.
  /// \sa PythonQt::InitFlags
  void setInitializationFlags(int flags);

  /// \sa setInitializationFlags
  int initializationFlags()const;

  /// Initialize python context considering the initializationFlags.
  /// Return \a True if python has been successfully initialized.
  /// \sa setInitializationFlags, mainContext, isPythonInitialized
  /// \sa preInitialization, executeInitializationScripts, pythonPreInitialized, pythonInitialized
  bool initialize();

  /// Return a reference to the python main context.
  /// Calling this function implicitly call initialize() if it hasn't been done.
  PythonQtObjectPtr mainContext();

  void addObjectToPythonMain(const QString& name, QObject* obj);
  void registerPythonQtDecorator(QObject* decorator);
  void registerClassForPythonQt(const QMetaObject* metaobject);
  void registerCPPClassForPythonQt(const char* name);

  /// This enum maps to Py_eval_input, Py_file_input and Py_single_input
  /// \see http://docs.python.org/c-api/veryhigh.html#Py_eval_input
  /// \see http://docs.python.org/c-api/veryhigh.html#Py_file_input
  /// \see http://docs.python.org/c-api/veryhigh.html#Py_single_input
  enum ExecuteStringMode
    {
    EvalInput = 0,
    FileInput,
    SingleInput
    };

  /// Execute a python of python code (can be multiple lines separated with newline)
  /// and return the result as a QVariant.
  QVariant executeString(const QString& code, ExecuteStringMode mode = FileInput);

  /// Gets the value of the variable looking in the __main__ module.
  /// If the variable is not found returns a default initialized QVariant.
  QVariant getVariable(const QString& varName);

  /// Execute a python script with the given filename.
  void executeFile(const QString& filename);

  /// Set function that is initialized after preInitialization and before executeInitializationScripts
  /// \sa preInitialization executeInitializationScripts
  void setInitializationFunction(void (*initFunction)());

  /// Given a python variable name, lookup its attributes and return them in a string list.
  /// By default the attributes are looked up from \c __main__.
  /// If the argument \c appendParenthesis is set to True, "()" will be appended to attributes
  /// being Python callable.
  QStringList pythonAttributes(const QString& pythonVariableName,
                               const QString& module = QLatin1String("__main__"),
                               bool appendParenthesis = false) const;

  /// Returns True if python is initialized
  /// \sa pythonInitialized
  bool isPythonInitialized()const;

  /// Returns True if a python error occured.
  /// \sa PythonQt::errorOccured()
  bool pythonErrorOccured()const;

Q_SIGNALS:

  /// This signal is emitted after python is pre-initialized. Observers can listen
  /// for this signal to handle additional initialization steps.
  /// \sa preInitialization
  void pythonPreInitialized();

  /// This signal is emitted after python is initialized and scripts are executed
  /// \sa preInitialization
  /// \sa executeScripts
  void pythonInitialized();

protected Q_SLOTS:
  void printStderr(const QString&);
  void printStdout(const QString&);

protected:

  void initPythonQt(int flags);

  virtual QStringList     pythonPaths();

  /// Overload this function to load Decorator and pythonQt wrapper at initialization time
  virtual void            preInitialization();

  /// Overload this function to execute script at initialization time
  virtual void            executeInitializationScripts();

protected:
  QScopedPointer<ctkAbstractPythonManagerPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(ctkAbstractPythonManager);
  Q_DISABLE_COPY(ctkAbstractPythonManager);

};
#endif
