#include <QByteArray>
#include <QFile>
#include <QtCharts/QtCharts>
#include <QtCore/QTimeLine>
#include <QtGui/QTextBlockFormat>
#include <QtGui/QTextCursor>
#include <QtSvg/QGraphicsSvgItem>
#include <QtSvg/QSvgGenerator>
#include <QtSvg/QSvgRenderer>
#include <QtWidgets/QGraphicsProxyWidget>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <utility>
#include <utils/PlannerUtils.hpp>

#include "QtVisualizer.h"

#include "base/Environment.h"
#include "base/PlannerSettings.h"
#include "base/Primitives.h"

QApplication *QtVisualizer::_app = nullptr;
QMainWindow *QtVisualizer::_window = nullptr;
VisualizationView *QtVisualizer::_view = nullptr;
QGraphicsScene *QtVisualizer::_scene = nullptr;

std::vector<LegendEntry> QtVisualizer::_legend;
std::list<QGraphicsItem *> QtVisualizer::_storedItems;

bool QtVisualizer::_showStartGoal = true;

void QtVisualizer::initialize() {
  int argc = 0;
  _app = new QApplication(argc, nullptr);
  _window = new QMainWindow;
  _scene = new QGraphicsScene;
  _view = new VisualizationView(_scene);
  _window->setCentralWidget(_view);

  _scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  _view->setScene(_scene);
  _view->setCacheMode(QGraphicsView::CacheBackground);
  _view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
  _view->setRenderHint(QPainter::Antialiasing);
  _view->setMinimumSize(500, 500);

  //    _window->setWindowFlags(Qt::WindowStaysOnTopHint);
  _window->setWindowTitle(QString("Motion Planning Visualizer"));
  _window->show();
}

QtVisualizer::~QtVisualizer() {
  delete _view;
  delete _app;
  delete _window;
  delete _scene;
}

int QtVisualizer::exec() { return _app->exec(); }

void QtVisualizer::visualize(Environment *environment, int run,
                             bool renderDistances) {
  if (_scene == nullptr) initialize();
  _window->setWindowTitle(
      QString("Theta* Trajectory Planning (run %1)").arg(run));
  _scene->setSceneRect(0, 0, environment->width() + 1,
                       environment->height() + 1);
  float scale = (float)1080 * .9f / (float)environment->width();
  _view->scale(scale, scale);
  QPen pen(QColor(128, 128, 128, 20));
  pen.setWidthF(0.01f);
  pen.setCapStyle(Qt::PenCapStyle::FlatCap);
  pen.setJoinStyle(Qt::PenJoinStyle::BevelJoin);
  for (unsigned x = 0;
       x <= std::max(environment->width(), environment->height()); ++x) {
    //        _scene->addLine(x, 0, x, environment->height(), pen);
    //        _scene->addLine(0, x, environment->width(), x, pen);
    if (x % 5 == 0) {
      QPen blackPen(Qt::black);
      blackPen.setWidthF(0.01f);

      if (x <= environment->width()) {
        //        QLabel *htext = new
        //        QLabel(QString::fromStdString(std::to_string(x)));
        //        htext->setFont(QFont("Consolas", 1));
        //        htext->setAlignment(Qt::AlignLeft);
        //        htext->setAttribute(Qt::WA_TranslucentBackground);
        //        auto * widget = _scene->addWidget(htext);
        //        widget->setGeometry(QRectF(x + 0.15, -1.2, 2, 2));
        _scene->addLine(x, -2, x, 0, blackPen);
      }

      if (x <= environment->height()) {
        //        QLabel *vtext = new
        //        QLabel(QString::fromStdString(std::to_string(x)));
        //        vtext->setFont(QFont("Consolas", 1));
        //        vtext->setAlignment(Qt::AlignRight);
        //        vtext->setAttribute(Qt::WA_TranslucentBackground);
        //        _scene->addWidget(vtext)->setGeometry(QRectF(-2.2, x, 2, 2));
        _scene->addLine(-2, x, 0, x, blackPen);
      }
    }

    for (unsigned y = 0; y <= environment->height(); ++y) {
      if (environment->collides(x, y))
        _scene->addRect(x, y, 1, 1, pen, QColor(128, 128, 128));
      //            _scene->addEllipse(x-.25, y-.25, 0.4, 0.4, pen,
      //                            QColor::fromHslF(std::max(0., std::min(.65,
      //                            environment->distance(x, y) * .1)), 1.,
      //                            .6));
      else if (renderDistances) {
        _scene->addRect(
            x, y, 0.5, 0.5, pen,
            QColor::fromHslF(
                std::min(.65,
                         environment->bilinearDistance(x + .25, y + .25) * .1),
                1., .6));
        _scene->addRect(
            x + 0.5, y, 0.5, 0.5, pen,
            QColor::fromHslF(
                std::min(.65,
                         environment->bilinearDistance(x + .75, y + .25) * .1),
                1., .6));
        _scene->addRect(
            x, y + 0.5, 0.5, 0.5, pen,
            QColor::fromHslF(
                std::min(.65,
                         environment->bilinearDistance(x + .25, y + .75) * .1),
                1., .6));
        _scene->addRect(
            x + 0.5, y + 0.5, 0.5, 0.5, pen,
            QColor::fromHslF(
                std::min(.65,
                         environment->bilinearDistance(x + .75, y + .75) * .1),
                1., .6));
      }
    }
  }

  if (_showStartGoal) {
    drawNode(environment->start(), QColor(200, 60, 0), 0.8);
    drawNode(environment->goal(), QColor(0, 80, 200), 0.8);
  }
}

void QtVisualizer::drawNode(const ompl::base::State *node, const QColor &color,
                            double radius, bool drawArrow) {
  if (_scene == nullptr) initialize();
  const auto *node2d = node->as<ompl::base::SE2StateSpace::StateType>();
  if (drawArrow) {
    QPen pen(color);
    pen.setWidthF(radius * 0.5);
    _scene->addLine(node2d->getX(), node2d->getY(),
                    node2d->getX() + std::cos(node2d->getYaw()) * radius * 5.,
                    node2d->getY() + std::sin(node2d->getYaw()) * radius * 5.,
                    pen);
  }
  drawNode(node2d->getX(), node2d->getY(), color, radius);
}

void QtVisualizer::drawNode(const Point &point, const QColor &color,
                            double radius) {
  drawNode(point.x, point.y, color, radius);
}

void QtVisualizer::drawNode(double x, double y, const QColor &color,
                            double radius) {
  if (_scene == nullptr) initialize();
  QPen pen(color);
  pen.setWidthF(0.);
  _scene->addEllipse(x - radius, y - radius, 2 * radius, 2 * radius, pen,
                     QBrush(color));
}

void QtVisualizer::drawNode(double x, double y, double theta,
                            const QColor &color, double radius,
                            bool drawArrow) {
  if (_scene == nullptr) initialize();
  if (drawArrow) {
    QPen pen(color);
    pen.setWidthF(radius * 0.5);
    _scene->addLine(x, y, x + std::cos(theta) * radius * 5.,
                    y + std::sin(theta) * radius * 5., pen);
  }
  drawNode(x, y, color, radius);
}

void QtVisualizer::drawTrajectory(const ompl::geometric::PathGeometric &path,
                                  const QColor &color, float penWidth,
                                  Qt::PenStyle penStyle) {
  drawPath(path, color, penWidth, penStyle);
}

void QtVisualizer::drawTrajectory(const ompl::base::State *a,
                                  const ompl::base::State *b,
                                  const QColor &color, float penWidth,
                                  Qt::PenStyle penStyle) {
  ompl::geometric::PathGeometric path(global::settings.ompl.space_info, a, b);
  drawPath(path, color, penWidth, penStyle);
}

void QtVisualizer::drawPath(const ompl::geometric::PathGeometric &p,
                            const QColor &color, float penWidth,
                            Qt::PenStyle penStyle) {
  ompl::geometric::PathGeometric path = PlannerUtils::interpolated(p);
  if (path.getStates().empty()) return;
  if (_scene == nullptr) initialize();
  QPen pen(color);
  pen.setWidthF(0.05f * penWidth);
  pen.setStyle(penStyle);
  QPainterPath pp;
  const auto *start = path.getStates()[0]->as<State>();
  pp.moveTo(start->getX(), start->getY());
  for (const auto *state : path.getStates()) {
    const auto *s = state->as<State>();
    pp.lineTo(s->getX(), s->getY());
  }
  _scene->addPath(pp, pen);
}

void QtVisualizer::drawPath(const std::vector<Point> &nodes,
                            const QColor &color, float penWidth,
                            Qt::PenStyle penStyle) {
  if (nodes.empty()) return;
  if (_scene == nullptr) initialize();
  QPen pen(color);
  pen.setWidthF(0.05f * penWidth);
  pen.setStyle(penStyle);
  QPainterPath pp;
  pp.moveTo(nodes[0].x, nodes[0].y);
  for (unsigned int i = 1; i < nodes.size(); ++i)
    pp.lineTo(nodes[i].x, nodes[i].y);
  _scene->addPath(pp, pen);
}

void QtVisualizer::drawPath(const std::vector<Point> &nodes, QPen pen) {
  if (nodes.empty()) return;
  if (_scene == nullptr) initialize();
  pen.setWidthF(pen.widthF() * 0.05f);
  QPainterPath pp;
  pp.moveTo(nodes[0].x, nodes[0].y);
  for (unsigned int i = 1; i < nodes.size(); ++i)
    pp.lineTo(nodes[i].x, nodes[i].y);
  _scene->addPath(pp, pen);
}

void QtVisualizer::drawNodes(ompl::geometric::PathGeometric path,
                             bool drawArrows, const QColor &color,
                             double radius) {
  for (const auto *node : path.getStates())
    drawNode(node, color, radius, drawArrows);
}

void QtVisualizer::drawLabel(const std::string &text, double x, double y,
                             const QColor &color, float size) {
  if (_scene == nullptr) initialize();
  QFont font("Consolas", 12);
  auto *textItem = _scene->addText(QString::fromStdString(text), font);
  textItem->setPos(x, y);
  textItem->setScale(size * 0.05);
  textItem->setDefaultTextColor(color);
}

void QtVisualizer::drawNodes(const std::vector<Point> &nodes,
                             const QColor &color, double radius) {
  for (auto &node : nodes) drawNode(node, color, radius);
}

void QtVisualizer::addLegendEntry(LegendEntry entry) {
  entry.pen.setWidthF(entry.pen.widthF() * 0.05f);
  _legend.push_back(entry);
}

void QtVisualizer::drawLegend() {
  if (_scene == nullptr) initialize();
  double x = global::settings.environment->width() + 3;
  double y = 0;
  _scene->addRect(x, y, 10,
                  (_legend.size() + (_showStartGoal ? 2 : 0)) * 1. + 0.2,
                  QPen(Qt::black, 0.01), QBrush(QColor(255, 255, 255, 220)));
  y += 0.2;
  QFont font("Arial", 10);
  for (auto &entry : _legend) {
    _scene->addLine(x + .3, y + .4, x + 1.5, y + .4, entry.pen);
    auto *textItem = _scene->addText(QString::fromStdString(entry.label), font);
    textItem->setScale(0.05);
    textItem->setPos(x + 1.7, y - 0.2);
    textItem->setDefaultTextColor(Qt::black);
    y += 1;
  }

  if (_showStartGoal) {
    drawNode(x + 0.9, y + 0.37, QColor(200, 60, 0));
    auto *sti = _scene->addText(QString("Start"), font);
    sti->setScale(0.05);
    sti->setPos(x + 1.7, y - 0.2);
    sti->setDefaultTextColor(Qt::black);
    y += 1;
    drawNode(x + 0.9, y + 0.37, QColor(0, 80, 200));
    auto *gti = _scene->addText(QString("Goal"), font);
    gti->setScale(0.05);
    gti->setPos(x + 1.7, y - 0.2);
    gti->setDefaultTextColor(Qt::black);
  }
}

void QtVisualizer::showStartGoal(bool show) { _showStartGoal = show; }

void QtVisualizer::savePng(const QString &fileName) {
  if (_scene == nullptr) initialize();
  QRectF newSceneRect;
  for (auto *item : _scene->items()) {
    newSceneRect |= item->mapToScene(item->boundingRect()).boundingRect();
  }
  _scene->setSceneRect(newSceneRect);
  _scene->clearSelection();

  QImage image(_scene->sceneRect().size().toSize().scaled(2000, 2000,
                                                          Qt::KeepAspectRatio),
               QImage::Format_ARGB32);
  image.fill(Qt::white);

  QPainter painter(&image);
  _scene->render(&painter);
  image.save(fileName);

  std::cout << "Saved PNG at " << fileName.toStdString() << std::endl;
}

void QtVisualizer::saveSvg(const QString &fileName) {
  if (_scene == nullptr) initialize();
  QRectF newSceneRect;
  for (auto *item : _scene->items()) {
    newSceneRect |= item->mapToScene(item->boundingRect()).boundingRect();
  }
  _scene->setSceneRect(newSceneRect);
  _scene->clearSelection();
  QSize sceneSize = newSceneRect.size().toSize();
  sceneSize.setWidth(sceneSize.width() * 10);
  sceneSize.setHeight(sceneSize.height() * 10);

  QSvgGenerator generator;
  generator.setFileName(fileName);
  generator.setSize(sceneSize);
  generator.setViewBox(QRect(0, 0, sceneSize.width(), sceneSize.height()));
  generator.setDescription(QObject::tr("Post-smoothing Trajectories"));
  generator.setTitle(fileName);
  QPainter painter;
  painter.begin(&generator);
  _scene->render(&painter);
  painter.end();

  std::cout << "Saved SVG at " << fileName.toStdString() << std::endl;
}

void QtVisualizer::saveScene() {
  _storedItems.clear();
  for (auto *item : _scene->items().toStdList()) {
    _storedItems.push_back(item);
  }
}

void QtVisualizer::restoreScene() {
  for (auto *sceneItem : _scene->items().toStdList()) {
    bool found = false;
    for (auto *item : _storedItems) {
      if (item == sceneItem) {
        found = true;
        break;
      }
    }
    if (!found) _scene->removeItem(sceneItem);
  }
  _storedItems.clear();
}

VisualizationView::VisualizationView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(parent), _numScheduledScalings(1), _scene(scene) {
  setDragMode(QGraphicsView::ScrollHandDrag);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showContextMenu(const QPoint &)));
}

VisualizationView::~VisualizationView() { delete _scene; }

void VisualizationView::wheelEvent(QWheelEvent *event) {
  if (event->delta() < 0)
    scale(0.9, 0.9);
  else
    scale(1.1, 1.1);
}

void VisualizationView::showContextMenu(const QPoint &pos) {
  QMenu contextMenu("Context menu", this);

  QAction action1("Save as SVG", this);
  connect(&action1, SIGNAL(triggered()), this, SLOT(saveSvg()));
  contextMenu.addAction(&action1);

  QAction action2("Save as PNG", this);
  connect(&action2, SIGNAL(triggered()), this, SLOT(savePng()));
  contextMenu.addAction(&action2);

  contextMenu.exec(mapToGlobal(pos));
}

void VisualizationView::saveSvg() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Save SVG", "", "Scalable Vector Graphics (*.svg)");
  if (filename.isNull()) return;

  QtVisualizer::saveSvg(filename);
}

void VisualizationView::savePng() {
  QString filename = QFileDialog::getSaveFileName(
      this, "Save PNG", "", "Portable Network Graphics (*.png)");
  if (filename.isNull()) return;

  QtVisualizer::savePng(filename);
}
