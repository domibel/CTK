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

// Qt includes
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>

// CTK includes
#include "ctkVTKAbstractView.h"
#include "ctkVTKAbstractView_p.h"
#include "ctkLogger.h"

// VTK includes
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTextProperty.h>

//--------------------------------------------------------------------------
static ctkLogger logger("org.commontk.visualization.vtk.widgets.ctkVTKAbstractView");
//--------------------------------------------------------------------------

// --------------------------------------------------------------------------
// ctkVTKAbstractViewPrivate methods

// --------------------------------------------------------------------------
ctkVTKAbstractViewPrivate::ctkVTKAbstractViewPrivate(ctkVTKAbstractView& object)
  : q_ptr(&object)
{
  this->RenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  this->CornerAnnotation = vtkSmartPointer<vtkCornerAnnotation>::New();
  this->RequestTimer = 0;
  this->RenderEnabled = true;
}

// --------------------------------------------------------------------------
void ctkVTKAbstractViewPrivate::init()
{
  Q_Q(ctkVTKAbstractView);

  this->setParent(q);

  this->VTKWidget = new QVTKWidget;
  q->setLayout(new QVBoxLayout);
  q->layout()->setMargin(0);
  q->layout()->setSpacing(0);
  q->layout()->addWidget(this->VTKWidget);

  this->RequestTimer = new QTimer(q);
  this->RequestTimer->setSingleShot(true);
  QObject::connect(this->RequestTimer, SIGNAL(timeout()),
                   q, SLOT(forceRender()));

  this->setupCornerAnnotation();
  this->setupRendering();

  // block renders and observe interactor to enforce framerate
  q->setInteractor(this->RenderWindow->GetInteractor());
}

// --------------------------------------------------------------------------
void ctkVTKAbstractViewPrivate::setupCornerAnnotation()
{
  this->CornerAnnotation->SetMaximumLineHeight(0.07);
  vtkTextProperty *tprop = this->CornerAnnotation->GetTextProperty();
  tprop->ShadowOn();
  this->CornerAnnotation->ClearAllTexts();
}

//---------------------------------------------------------------------------
void ctkVTKAbstractViewPrivate::setupRendering()
{
  Q_ASSERT(this->RenderWindow);
  this->RenderWindow->SetAlphaBitPlanes(1);
  this->RenderWindow->SetMultiSamples(0);
  this->RenderWindow->StereoCapableWindowOn();

  this->VTKWidget->SetRenderWindow(this->RenderWindow);
}

//---------------------------------------------------------------------------
QList<vtkRenderer*> ctkVTKAbstractViewPrivate::renderers()const
{
  QList<vtkRenderer*> rendererList;

  vtkRendererCollection* rendererCollection = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator rendererIterator;
  rendererCollection->InitTraversal(rendererIterator);
  vtkRenderer *renderer;
  while ( (renderer= rendererCollection->GetNextRenderer(rendererIterator)) )
    {
    rendererList << renderer;
    }
  return rendererList;
}

//---------------------------------------------------------------------------
vtkRenderer* ctkVTKAbstractViewPrivate::firstRenderer()const
{
  return static_cast<vtkRenderer*>(this->RenderWindow->GetRenderers()
    ->GetItemAsObject(0));
}

//---------------------------------------------------------------------------
// ctkVTKAbstractView methods

// --------------------------------------------------------------------------
ctkVTKAbstractView::ctkVTKAbstractView(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new ctkVTKAbstractViewPrivate(*this))
{
  Q_D(ctkVTKAbstractView);
  d->init();
}

// --------------------------------------------------------------------------
ctkVTKAbstractView::ctkVTKAbstractView(ctkVTKAbstractViewPrivate* pimpl, QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(pimpl)
{
  // derived classes must call init manually. Calling init() here may results in
  // actions on a derived public class not yet finished to be created
}

//----------------------------------------------------------------------------
ctkVTKAbstractView::~ctkVTKAbstractView()
{
}

//----------------------------------------------------------------------------
void ctkVTKAbstractView::scheduleRender()
{
  Q_D(ctkVTKAbstractView);

  //logger.trace(QString("scheduleRender - RenderEnabled: %1 - Request render elapsed: %2ms").
  //             arg(d->RenderEnabled ? "true" : "false")
  //             .arg(d->RequestTime.elapsed()));

  if (!d->RenderEnabled)
    {
    return;
    }

  double msecsBeforeRender = 100. / d->RenderWindow->GetDesiredUpdateRate();
  if(d->VTKWidget->testAttribute(Qt::WA_WState_InPaintEvent))
    {
    // If the request comes from the system (widget exposed, resized...), the
    // render must be done immediately.
    this->forceRender();
    }
  else if (!d->RequestTime.isValid())
    {
    // If the DesiredUpdateRate is in "still mode", the requested framerate
    // is fake, it is just a way to allocate as much time as possible for the
    // rendering, it doesn't really mean that rendering must occur only once
    // every couple seconds. It just means it should be done when there is
    // time to do it. A timer of 0, kind of mean a rendering is done next time
    // it is idle.
    if (msecsBeforeRender > 10000)
      {
      msecsBeforeRender = 0;
      }
    d->RequestTime.start();
    d->RequestTimer->start(static_cast<int>(msecsBeforeRender));
    }
  else if (d->RequestTime.elapsed() > msecsBeforeRender)
    {
    // The rendering hasn't still be done, but msecsBeforeRender milliseconds
    // have already been elapsed, it is likely that RequestTimer has already
    // timed out, but the event queue hasn't been processed yet, rendering is
    // done now to ensure the desired framerate is respected.
    this->forceRender();
    }
}

//----------------------------------------------------------------------------
void ctkVTKAbstractView::forceRender()
{
  Q_D(ctkVTKAbstractView);

  if (this->sender() == d->RequestTimer  &&
      !d->RequestTime.isValid())
    {
    // The slot associated to the timeout signal is now called, however the
    // render has already been executed meanwhile. There is no need to do it
    // again.
    return;
    }

  // The timer can be stopped if it hasn't timed out yet.
  d->RequestTimer->stop();
  d->RequestTime = QTime();

  //logger.trace(QString("forceRender - RenderEnabled: %1")
  //             .arg(d->RenderEnabled ? "true" : "false"));

  if (!d->RenderEnabled || !this->isVisible())
    {
    return;
    }
  d->RenderWindow->Render();
}

//----------------------------------------------------------------------------
CTK_GET_CPP(ctkVTKAbstractView, vtkRenderWindow*, renderWindow, RenderWindow);

//----------------------------------------------------------------------------
void ctkVTKAbstractView::setInteractor(vtkRenderWindowInteractor* newInteractor)
{
  Q_D(ctkVTKAbstractView);

  d->RenderWindow->SetInteractor(newInteractor);
  // Prevent the interactor to call Render() on the render window; only
  // scheduleRender() and forceRender() can Render() the window.
  // This is done to ensure the desired framerate is respected.
  newInteractor->SetEnableRender(false);
  qvtkReconnect(d->RenderWindow->GetInteractor(), newInteractor,
                vtkCommand::RenderEvent, this, SLOT(scheduleRender()));
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* ctkVTKAbstractView::interactor()const
{
  Q_D(const ctkVTKAbstractView);
  return d->RenderWindow->GetInteractor();
}

//----------------------------------------------------------------------------
vtkInteractorObserver* ctkVTKAbstractView::interactorStyle()const
{
  return this->interactor() ?
    this->interactor()->GetInteractorStyle() : 0;
}

//----------------------------------------------------------------------------
void ctkVTKAbstractView::setCornerAnnotationText(const QString& text)
{
  Q_D(ctkVTKAbstractView);
  d->CornerAnnotation->ClearAllTexts();
  d->CornerAnnotation->SetText(2, text.toLatin1());
}

//----------------------------------------------------------------------------
QString ctkVTKAbstractView::cornerAnnotationText() const
{
  Q_D(const ctkVTKAbstractView);
  return QLatin1String(d->CornerAnnotation->GetText(2));
}

//----------------------------------------------------------------------------
vtkCornerAnnotation* ctkVTKAbstractView::cornerAnnotation() const
{
  Q_D(const ctkVTKAbstractView);
  return d->CornerAnnotation;
}

//----------------------------------------------------------------------------
QVTKWidget * ctkVTKAbstractView::VTKWidget() const
{
  Q_D(const ctkVTKAbstractView);
  return d->VTKWidget;
}

//----------------------------------------------------------------------------
CTK_SET_CPP(ctkVTKAbstractView, bool, setRenderEnabled, RenderEnabled);
CTK_GET_CPP(ctkVTKAbstractView, bool, renderEnabled, RenderEnabled);

//----------------------------------------------------------------------------
QSize ctkVTKAbstractView::minimumSizeHint()const
{
  // Arbitrary size. 50x50 because smaller seems too small.
  return QSize(50, 50);
}

//----------------------------------------------------------------------------
QSize ctkVTKAbstractView::sizeHint()const
{
  // Arbitrary size. 300x300 is the default vtkRenderWindow size.
  return QSize(300, 300);
}

//----------------------------------------------------------------------------
bool ctkVTKAbstractView::hasHeightForWidth()const
{
  return true;
}

//----------------------------------------------------------------------------
int ctkVTKAbstractView::heightForWidth(int width)const
{
  // typically VTK render window tend to be square...
  return width;
}

//----------------------------------------------------------------------------
void ctkVTKAbstractView::setBackgroundColor(const QColor& newBackgroundColor)
{
  Q_D(ctkVTKAbstractView);
  double color[3];
  color[0] = newBackgroundColor.redF();
  color[1] = newBackgroundColor.greenF();
  color[2] = newBackgroundColor.blueF();
  foreach(vtkRenderer* renderer, d->renderers())
    {
    renderer->SetBackground(color);
    }
}

//----------------------------------------------------------------------------
QColor ctkVTKAbstractView::backgroundColor()const
{
  Q_D(const ctkVTKAbstractView);
  vtkRenderer* firstRenderer = d->firstRenderer();
  return firstRenderer ? QColor::fromRgbF(firstRenderer->GetBackground()[0],
                                          firstRenderer->GetBackground()[1],
                                          firstRenderer->GetBackground()[2])
                       : QColor();
}

//----------------------------------------------------------------------------
void ctkVTKAbstractView::setBackgroundColor2(const QColor& newBackgroundColor)
{
  Q_D(ctkVTKAbstractView);
  double color[3];
  color[0] = newBackgroundColor.redF();
  color[1] = newBackgroundColor.greenF();
  color[2] = newBackgroundColor.blueF();
  foreach(vtkRenderer* renderer, d->renderers())
    {
    renderer->SetBackground2(color);
    }
}

//----------------------------------------------------------------------------
QColor ctkVTKAbstractView::backgroundColor2()const
{
  Q_D(const ctkVTKAbstractView);
  vtkRenderer* firstRenderer = d->firstRenderer();
  return firstRenderer ? QColor::fromRgbF(firstRenderer->GetBackground2()[0],
                                          firstRenderer->GetBackground2()[1],
                                          firstRenderer->GetBackground2()[2])
                       : QColor();
}

//----------------------------------------------------------------------------
void ctkVTKAbstractView::setGradientBackground(bool enable)
{
  Q_D(ctkVTKAbstractView);
  foreach(vtkRenderer* renderer, d->renderers())
    {
    renderer->SetGradientBackground(enable);
    }
}

//----------------------------------------------------------------------------
bool ctkVTKAbstractView::gradientBackground()const
{
  Q_D(const ctkVTKAbstractView);
  vtkRenderer* firstRenderer = d->firstRenderer();
  return firstRenderer ? firstRenderer->GetGradientBackground() : false;
}
