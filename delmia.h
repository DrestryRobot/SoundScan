#ifndef DELMIA_H
#define DELMIA_H

#include <QAxObject>

class delmia
{
public:
    delmia();

private:

    QAxObject* catia;
    QAxObject* windows;
    QAxObject* documents;
    QAxObject* doc;
    QAxObject* partDocument;
    QAxObject* pprdocument;
    QAxObject* part;
    QAxObject* partDoc;
    QAxObject* processDoc;
    QAxObject* productDoc;
    QAxObject* product;
    QAxObject* hybridShapeFactory;
    QAxObject* hybridBodies;
    QAxObject* hybridBody2;
    QAxObject* hybridShape;
    QAxObject* hybridShapeBoundary;
    QAxObject* hybridShapeAssemble;
    QAxObject* hybridShapeAssemble1;
    QAxObject* hybridShapeAssemble2;
    QAxObject* hybridShapeAssemble3;
    QAxObject* hybridShapeAssemble4;
    QAxObject* hybridShapeExtract;
    QAxObject* hybridShapeExtrapol1;
    QAxObject* hybridShapeExtrapol2;
    QAxObject* hybridShapeExtractref3;
    QAxObject* hybridShapePointOnCurve1;
    QAxObject* hybridShapePointOnCurve2;
    QAxObject* hybridShapePointOnCurve3;
    QAxObject* hybridShapePointOnCurve4;
    QAxObject* hybridShapePointOnCurve5;
    QAxObject* hybridShapePointOnCurveRef1;
    QAxObject* hybridShapePointOnCurveRef2;
    QAxObject* hybridShapePointOnCurvetest;
    QAxObject* hybridShapePointOnCurvetestbegin;
    QAxObject* hybridShapePointOnCurvebegin;
    QAxObject* hybridShapePointOnCurveend;
    QAxObject* hybridShapeSplit1;
    QAxObject* hybridShapeSplit2;
    QAxObject* hybridShapeSplit3;
    QAxObject* hybridShapeSplit4;
    QAxObject* hybridShapeSplit5;
    QAxObject* hybridShapeSplit6;
    QAxObject* hybridShapeSplit7;
    QAxObject* hybridShapeSplit8;
    QAxObject* hybridShapeSplit9;
    QAxObject* hybridShapeSplit10;
    QAxObject* hybridShapeSplit51;
    QAxObject* hybridShapeSplit52;
    QAxObject* hybridShapeSplit53;
    QAxObject* hybridShapeSplit61;
    QAxObject* hybridShapeSplit62;
    QAxObject* hybridShapeSplit63;
    QAxObject* hybridShapeSplit71;
    QAxObject* hybridShapeSplit72;
    QAxObject* hybridShapeSplit73;
    QAxObject* hybridShapeSplit81;
    QAxObject* hybridShapeSplit82;
    QAxObject* hybridShapeSplit83;
    QAxObject* hybridShapeSplit91;
    QAxObject* hybridShapeSplit92;
    QAxObject* hybridShapeSplit93;
    QAxObject* hybridShapeSplit101;
    QAxObject* hybridShapeSplit102;
    QAxObject* hybridShapeSplit103;
    QAxObject* hybridShapeIntersection1;
    QAxObject* hybridShapeIntersection2;
    QAxObject* hybridShapeIntersection3;
    QAxObject* hybridShapeIntersection31;
    QAxObject* hybridShapeIntersection32;
    QAxObject* hybridShapeIntersection33;
    QAxObject* hybridShapeIntersection34;
    QAxObject* hybridShapeIntersection35;
    QAxObject* hybridShapeIntersection36;
    QAxObject* hybridShapeIntersection37;
    QAxObject* hybridShapeLinePtPttest;
    QAxObject* hybridShapeLineTangency1;
    QAxObject* hybridShapeLineTangency2;
    QAxObject* hybridShapeCurvePartest;
    QAxObject* hybridShapeCurvePar1;
    QAxObject* hybridShapeCurvePar2;
    QAxObject* HybridShapeLineNormal1;
    QAxObject* selection;
    QAxObject* selectiontest;
    QAxObject* item;
    QAxObject* item1;
    QAxObject* item11;
    QAxObject* item12;
    QAxObject* value;
    QAxObject* visPropertySet;
    QAxObject* visPropertySet1;
    QAxObject* ref;
    QAxObject* ref1;
    QAxObject* ref11;
    QAxObject* ref12;
    QAxObject* mywindow;
    QAxObject* work;
    QAxObject* objRobot;
    QAxObject* longmen;
    QAxObject* reference;
    QAxObject* reference1;
    QAxObject* reference2;
    QAxObject* reference3;
    QAxObject* reference4;
    QAxObject* startpoint;
    QAxObject* endpoint;
    QAxObject* refbeginpoint;
    QAxObject* refstartpoint;
    QAxObject* TheSPAWorkbench1;
    QAxObject* TheSPAWorkbench2;
    QAxObject* TheMeasurable1;
    QAxObject* TheMeasurable2;
    QAxObject* TheMeasurable3;
    QAxObject* TheMeasurable4;
    QAxObject* TheMeasurable5;
    QAxObject* objRobotTask;
    QAxObject* objTagGroup;
    QAxObject* objRefOperation;
    QAxObject* objAfterOperation;
    QAxObject* objOperation;
    QAxObject* objRefAct;
    QAxObject* objRobotmotion;
    QAxObject* objTag;
    QAxObject* objRobotTaskFactory;
    QAxObject* objTagGroupFactory;
    QAxObject* objDevice;
    QAxObject* split3;

    QList<QAxObject*> heng;
    QList<QAxObject*> zong;
    QList<QAxObject*> heng1;
    QList<QAxObject*> zong1;
    QList<QAxObject*> edge;
    QList<QAxObject*> hybridShapeExtractref;
    QList<QAxObject*> hybridShapeExtractref1;
    QList<QAxObject*> hybridShapeExtractref2;
    QList<QAxObject*> activedoc;
    QList<QAxObject*> refpoint;
    QList<QAxObject*> split1;
    QList<QAxObject*> split2;
    QList<QAxObject*> split4;
    QList<QAxObject*> ref3;

    QVariantList beginpoint;
    QVariantList bp;
    QVariantList Direction;
    QVariantList Direction1;
    QVariantList Direction2;
    QVariantList Direction3;
    QVariantList oAxisComponentsArrayprocess;
    QVariantList ListOfDOFValues;
    QVariantList InputObject = { "BiDimInfinite" };

    int count = 0;
    int j = 1;
    int G = 0;
    int pointnum = 0;
    int hybridShapeSplitdir = -1;

    double l;
    double area;
    double d0;
    double d2;
    double t0;
    double t3;
    double Angle;
    double lenth;
    double lastlenth;
    double xx, yy, zz; // 单个坐标值
    QVector<double> pointx, pointy, pointz; // 坐标数组
    QVector<double> tagx, tagy, tagz; // 标签点坐标
    QVector<double> normalx, normaly, normalz; // 法向量
    QVector<double> zaxisx, zaxisy, zaxisz; // 坐标系 Z 轴
    QVector<double> xaxisx, xaxisy, xaxisz; // 坐标系 X 轴
    QVector<double> yaxisx, yaxisy, yaxisz; // 坐标系 Y 轴
    QVector<double> R, P, y; // 其他数组

    bool haveh = false;
    bool havez = false;
    bool only = false;
    bool test0 = false;
    bool test = false;
    bool continue1 = false;
    bool pointdir = true;
    bool linedir = true;
    bool H = false;
    bool intersectionSuccess = false;
};

#endif // DELMIA_H
