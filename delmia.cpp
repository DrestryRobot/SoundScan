#include "delmia.h"

delmia::delmia() {}



// 选择横向界限---------------------------------------------------------------
// void MainWindow::on_pushButton_2_clicked()
// {
// partDocument = catia->querySubObject("ActiveDocument");

// part = partDocument->querySubObject("Part");

// hybridShapeFactory = part->querySubObject("HybridShapeFactory");

// hybridBodies = part->querySubObject("HybridBodies");

// selection = partDocument->querySubObject("Selection");

// selection->dynamicCall("Clear()");

// QVariantList filter;
// filter << "BiDimFeatEdge" << "MonoDimInfinite";

// redirector->log("请选择横向分界线，按ESC取消!");
// QMessageBox::information(nullptr, "Info", "请选择横向分界线，按ESC取消!");

// QVariant result = selection->dynamicCall("SelectElement3(QVariant, QString, bool, int, bool)",
//                                          QVariant(filter), "请选择横向分界线", true, 2, false);

// status = result.toString();
// if (status == "Cancel" || status == "Undo") {
//     return;
// }

// if (status != "Redo") {
//     count = selection->property("Count").toInt();
//     ui->lineEdit->setText(QString("%1条横向分界线").arg(count));

//     heng.clear();
//     for (int i = 1; i <= count; i++)
//     {
//         item = selection->querySubObject("Item(int)", i);
//         value = item->querySubObject("Value");
//         heng.append(value);
//     }
//     haveh = (count > 0);
// }

// selection->dynamicCall("Clear()");
// }


// 选择纵向界限------------------------------------------------------------------
// void MainWindow::on_pushButton_5_clicked()
// {
// partDocument = catia->querySubObject("ActiveDocument");

// part = partDocument->querySubObject("Part");

// hybridShapeFactory = part->querySubObject("HybridShapeFactory");

// hybridBodies = part->querySubObject("HybridBodies");

// selection = partDocument->querySubObject("Selection");

// selection->dynamicCall("Clear()");

// QVariantList filter;
// filter << "BiDimFeatEdge" << "MonoDimInfinite";

// redirector->log("请选择纵向分界线，按ESC取消!");
// QMessageBox::information(nullptr, "Info", "请选择纵向分界线，按ESC取消!");

// result = selection->dynamicCall("SelectElement3(QVariant, QString, bool, int, bool)",
//                                          QVariant(filter), "请选择纵向分界线", true, 2, false);

// status = result.toString();
// if (status == "Cancel" || status == "Undo") {
//     return;
// }

// int count = 0;
// if (status != "Redo") {
//     count = selection->property("Count").toInt();
//     ui->lineEdit_5->setText(QString("%1条纵向分界线").arg(count));

//     zong.clear();
//     for (int i = 1; i <= count; i++)
//     {
//         item = selection->querySubObject("Item(int)", i);
//         value = item->querySubObject("Value");
//         zong.append(value);
//     }
//     havez = (count > 0);
// }

// selection->dynamicCall("Clear()");
// }


// 创建分区-----------------------------------------------------------------
// void MainWindow::on_pushButton_6_clicked()
// {
// partDocument = catia->querySubObject("ActiveDocument");

// part = partDocument->querySubObject("Part");

// hybridShapeFactory = part->querySubObject("HybridShapeFactory");

// hybridBodies = part->querySubObject("HybridBodies");

// hybridBody2 = hybridBodies->querySubObject("Add()");

// redirector->log("请选择扫描面，按ESC取消!");
// QMessageBox::information(nullptr, "Info", "请选择扫描面，按ESC取消!");

// selection = partDocument->querySubObject("Selection");

// selection->dynamicCall("Clear()");

// result = selection->dynamicCall("SelectElement2(QVariant, QString, bool)",
//                                 InputObject, "请选择扫描面", false);

// status = result.toString();
// if (status == "Cancel" || status == "Undo")
// {
//     return;
// }

// if (status != "Redo")
// {
//     item = selection->querySubObject("Item(int)", 1);
//     hybridShapeAssemble1 = item->querySubObject("Value");
// }

// selection->dynamicCall("Clear()");

// // 提取曲面边界
// hybridShapeBoundary = hybridShapeFactory->querySubObject("AddNewBoundaryOfSurface(QVariant)",
//                                                         QVariant::fromValue(hybridShapeAssemble1));
// hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeBoundary));

// part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeBoundary));

// part->dynamicCall("Update()");

// if (havez == true)
// {
//     zong1.resize(zong.size());
//     for (int i = 0; i < zong.size(); ++i)
//     {
//         item11 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                  QVariant::fromValue(zong[i]));
//         zong1[i] = item11;
//     }
// }

// if (haveh == true)
// {
//     heng1.resize(heng.size());
//     for (int i = 0; i < heng.size(); ++i)
//     {
//         item12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                  QVariant::fromValue(heng[i]));
//         heng1[i] = item12;
//     }
// }

// if(havez == true && haveh == true)
// {
//     // 用横截纵
//     for (int i = 0; i < heng.size(); i++)
//     {
//         for (int j = 0; j < zong.size(); j++)
//         {
//             hybridShapeSplit1 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                    QVariant::fromValue(zong[j]), QVariant::fromValue(heng[i]), 1);
//             hybridShapeSplit1->setProperty("ExtrapolationType", 1);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit1));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit1));

//             hybridShapeSplit2 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                    QVariant::fromValue(zong[j]), QVariant::fromValue(heng[i]), -1);
//             hybridShapeSplit2->setProperty("ExtrapolationType", 1);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit2));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit2));

//             part->dynamicCall("Update()");

//             if (!hybridShapeSplit1 || !hybridShapeSplit2)
//             {
//                 selection->dynamicCall("Clear()");
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeSplit1));
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeSplit2));
//                 selection->dynamicCall("Delete()");
//                 break;
//             }

//             hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                             QVariant::fromValue(zong[j]), 0);

//             if (i < heng.size()-1)
//             {
//                 hybridShapeIntersection1 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                               QVariant::fromValue(hybridShapeSplit1), QVariant::fromValue(heng[i+1]));
//                 hybridShapeIntersection1->setProperty("PointType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection1));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection1));

//                 part->dynamicCall("Update()");

//                 selection->dynamicCall("Clear()");
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection1));
//                 selection->dynamicCall("Delete()");

//                 if (hybridShapeIntersection1)
//                 {
//                     zong[j] = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                    QVariant::fromValue(hybridShapeSplit2));
//                 }
//                 else
//                 {
//                     zong[j] = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                    QVariant::fromValue(hybridShapeSplit2));
//                 }
//             }
//         }
//     }

//     // 用纵截横
//     for (int i = 0; i < zong1.size(); i++)
//     {
//         for (int j = 0; j < heng.size(); j++)
//         {
//             hybridShapeSplit3 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                    QVariant::fromValue(heng[j]), QVariant::fromValue(zong1[i]), 1);
//             hybridShapeSplit3->setProperty("ExtrapolationType", 1);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit3));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit3));

//             hybridShapeSplit4 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                    QVariant::fromValue(heng[j]), QVariant::fromValue(zong1[i]), -1);
//             hybridShapeSplit4->setProperty("ExtrapolationType", 1);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit4));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit4));

//             part->dynamicCall("Update()");

//             if (!hybridShapeSplit3 || !hybridShapeSplit4)
//             {
//                 selection->dynamicCall("Clear()");
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeSplit3));
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeSplit4));
//                 selection->dynamicCall("Delete()");
//                 break;
//             }

//             hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                             QVariant::fromValue(heng[j]), 0);

//             if (i < zong1.size() - 1)
//             {
//                 hybridShapeIntersection2 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                               QVariant::fromValue(hybridShapeSplit3), QVariant::fromValue(zong1[i+1]));
//                 hybridShapeIntersection2->setProperty("PointType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection2));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection2));
//                 part->dynamicCall("Update()");

//                 selection->dynamicCall("Clear()");
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection2));
//                 selection->dynamicCall("Delete()");

//                 if (hybridShapeIntersection2)
//                 {
//                     heng[j] = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                    QVariant::fromValue(hybridShapeSplit3));
//                 }
//                 else
//                 {
//                     heng[j] = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                    QVariant::fromValue(hybridShapeSplit4));
//                 }
//             }

//         }
//     }

//     // 分割曲面边界
//     ref1 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapeBoundary));

//     split1.resize(heng1.size());
//     for (int j = 0; j < heng1.size(); j++)
//     {
//         split1[j] = heng1[j];
//     }
//     split2.resize(zong1.size());
//     for (int j = 0; j < zong1.size(); j++)
//     {
//         split2[j] = zong1[j];
//     }
//     for (int j = 0; j < split1.size(); j++)
//     {

//         hybridShapeSplit5 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                QVariant::fromValue(ref1), QVariant::fromValue(split1[j]), 1);
//         hybridShapeSplit5->setProperty("ExtrapolationType", 1);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit5));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit5));

//         hybridShapeSplit6 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                QVariant::fromValue(ref1), QVariant::fromValue(split1[j]), -1);
//         hybridShapeSplit6->setProperty("ExtrapolationType", 1);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit6));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit6));

//         part->dynamicCall("Update()");

//         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                         QVariant::fromValue(ref1), 0);
//         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                         QVariant::fromValue(hybridShapeSplit5), 0);
//         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                         QVariant::fromValue(hybridShapeSplit6), 0);

//         if (split1.size() - 1 == 0)
//         {
//             j = j + 1;
//             split1.resize(j + 1);
//             split1[j] = heng1[0];
//             only = true;
//         }

//         if (j == 0)
//         {
//             hybridShapeIntersection31 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                           QVariant::fromValue(hybridShapeSplit5), QVariant::fromValue(split1[j+1]));
//             hybridShapeIntersection31->setProperty("PointType", 1);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection31));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection31));
//             part->dynamicCall("Update()");

//             selection->dynamicCall("Clear()");
//             selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection31));
//             selection->dynamicCall("Delete()");

//             if (hybridShapeIntersection31)
//             {
//                 ref1 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                             QVariant::fromValue(hybridShapeSplit5));
//                 ref11 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                              QVariant::fromValue(hybridShapeSplit6));
//             }
//             else
//             {
//                 ref1 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                             QVariant::fromValue(hybridShapeSplit6));
//                 ref11 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                              QVariant::fromValue(hybridShapeSplit5));
//             }

//             hybridShapeAssemble2 = hybridShapeFactory->querySubObject("AddNewJoin(QVariant, QVariant)",
//                                                                       QVariant::fromValue(ref11), QVariant::fromValue(split1[j]));
//             hybridShapeAssemble2->setProperty("SetConnex", 1);
//             hybridShapeAssemble2->setProperty("SetManifold", 1);
//             hybridShapeAssemble2->setProperty("SetSimplify", 0);
//             hybridShapeAssemble2->setProperty("SetSuppressMode", 0);
//             hybridShapeAssemble2->setProperty("SetDeviation", 0.001);
//             hybridShapeAssemble2->setProperty("SetAngularToleranceMode", 0);
//             hybridShapeAssemble2->setProperty("SetAngularTolerance", 0.5);
//             hybridShapeAssemble2->setProperty("SetFederationPropagation", 0);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeAssemble2));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeAssemble2));
//             part->dynamicCall("Update()");

//             ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                          QVariant::fromValue(hybridShapeAssemble2));

//             for (int k = 0; k < split2.size(); k++)
//             {
//                 hybridShapeSplit51 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                        QVariant::fromValue(ref12), QVariant::fromValue(split2[k]), 1);
//                 if (!hybridShapeSplit51) break;
//                 hybridShapeSplit51->setProperty("ExtrapolationType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit51));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit51));

//                 hybridShapeSplit61 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                        QVariant::fromValue(ref12), QVariant::fromValue(split2[k]), -1);
//                 if (!hybridShapeSplit61) break;
//                 hybridShapeSplit61->setProperty("ExtrapolationType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit61));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit61));

//                 part->dynamicCall("Update()");

//                 hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                 QVariant::fromValue(ref12), 0);
//                 if (!hybridShapeSplit51 || !hybridShapeSplit61)
//                 {
//                     selection->dynamicCall("Clear()");
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeSplit51));
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeSplit61));
//                     selection->dynamicCall("Delete()");
//                     break;
//                 }

//                 if (split2.size() - 1 == 0)
//                 {
//                     break;
//                 }

//                 if (k == 0)
//                 {
//                     hybridShapeIntersection32 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                                    QVariant::fromValue(hybridShapeSplit51), QVariant::fromValue(split2[k+1]));
//                     hybridShapeIntersection32->setProperty("PointType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection32));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection32));
//                     part->dynamicCall("Update()");

//                     selection->dynamicCall("Clear()");
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection32));
//                     selection->dynamicCall("Delete()");

//                     if (hybridShapeIntersection32)
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit61));
//                     }
//                     else
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit51));
//                     }
//                 }

//                 if (k > 0)
//                 {
//                     hybridShapeIntersection33 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                                    QVariant::fromValue(hybridShapeSplit51), QVariant::fromValue(split2[k-1]));
//                     hybridShapeIntersection33->setProperty("PointType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection33));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection33));
//                     part->dynamicCall("Update()");

//                     selection->dynamicCall("Clear()");
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection33));
//                     selection->dynamicCall("Delete()");

//                     if (hybridShapeIntersection33)
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit61));

//                         hybridShapeSplit71 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit51), QVariant::fromValue(split2[k-1]), -1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit51), 0);
//                         hybridShapeSplit71->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit71));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit71));

//                         hybridShapeSplit81 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit51), QVariant::fromValue(split2[k-1]), 1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit51), 0);
//                         hybridShapeSplit81->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit81));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit81));

//                         part->dynamicCall("Update()");
//                     }
//                     else
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit51));

//                         hybridShapeSplit91 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit61), QVariant::fromValue(split2[k-1]), -1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit61), 0);
//                         hybridShapeSplit91->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit91));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit91));

//                         hybridShapeSplit101 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit61), QVariant::fromValue(split2[k-1]), 1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit61), 0);
//                         hybridShapeSplit101->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit101));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit101));

//                         part->dynamicCall("Update()");
//                     }
//                 }
//             }

//         }


//         if (j > 0)
//         {
//             hybridShapeIntersection3 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit5), QVariant::fromValue(split1[j-1]));
//             hybridShapeIntersection3->setProperty("PointType", 1);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection3));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection3));
//             part->dynamicCall("Update()");

//             selection->dynamicCall("Clear()");
//             selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection3));
//             selection->dynamicCall("Delete()");

//             if (hybridShapeIntersection3)
//             {
//                 ref1 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                             QVariant::fromValue(hybridShapeSplit6));
//                 ref11 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                              QVariant::fromValue(hybridShapeSplit5));
//                 if (only == false)
//                 {
//                     hybridShapeSplit7 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                             QVariant::fromValue(hybridShapeSplit5), QVariant::fromValue(split1[j-1]), -1);
//                     hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                     QVariant::fromValue(hybridShapeSplit5), 0);
//                     hybridShapeSplit7->setProperty("ExtrapolationType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit7));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit7));

//                     hybridShapeSplit8 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                             QVariant::fromValue(hybridShapeSplit5), QVariant::fromValue(split1[j-1]), 1);
//                     hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                     QVariant::fromValue(hybridShapeSplit5), 0);
//                     hybridShapeSplit8->setProperty("ExtrapolationType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit8));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit8));

//                     part->dynamicCall("Update()");
//                 }
//             }
//             else
//             {
//                 ref1 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                             QVariant::fromValue(hybridShapeSplit5));
//                 ref11 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                              QVariant::fromValue(hybridShapeSplit6));
//                 if (only == false)
//                 {
//                     hybridShapeSplit9 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                            QVariant::fromValue(hybridShapeSplit6), QVariant::fromValue(split1[j-1]), -1);
//                     hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                     QVariant::fromValue(hybridShapeSplit6), 0);
//                     hybridShapeSplit9->setProperty("ExtrapolationType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit9));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit9));

//                     hybridShapeSplit10 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                            QVariant::fromValue(hybridShapeSplit6), QVariant::fromValue(split1[j-1]), 1);
//                     hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                     QVariant::fromValue(hybridShapeSplit6), 0);
//                     hybridShapeSplit10->setProperty("ExtrapolationType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit10));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit10));

//                     part->dynamicCall("Update()");
//                 }
//             }

//             hybridShapeAssemble3 = hybridShapeFactory->querySubObject("AddNewJoin(QVariant, QVariant)",
//                                                                       QVariant::fromValue(ref11), QVariant::fromValue(split1[j]));

//             hybridShapeAssemble3->dynamicCall("AddElement(QVariant)", QVariant::fromValue(split1[j-1]));

//             hybridShapeAssemble3->setProperty("SetConnex", 1);
//             hybridShapeAssemble3->setProperty("SetManifold", 1);
//             hybridShapeAssemble3->setProperty("SetSimplify", 0);
//             hybridShapeAssemble3->setProperty("SetSuppressMode", 0);
//             hybridShapeAssemble3->setProperty("SetDeviation", 0.001);
//             hybridShapeAssemble3->setProperty("SetAngularToleranceMode", 0);
//             hybridShapeAssemble3->setProperty("SetAngularTolerance", 0.5);
//             hybridShapeAssemble3->setProperty("SetFederationPropagation", 0);

//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeAssemble3));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeAssemble3));
//             part->dynamicCall("Update()");

//             ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                          QVariant::fromValue(hybridShapeAssemble3));

//             for (int k = 0; k < split2.size(); k++)
//             {
//                 hybridShapeSplit52 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                         QVariant::fromValue(ref12), QVariant::fromValue(split2[k]), 1);
//                 hybridShapeSplit52->setProperty("ExtrapolationType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit52));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit52));

//                 hybridShapeSplit62 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                         QVariant::fromValue(ref12), QVariant::fromValue(split2[k]), -1);
//                 hybridShapeSplit62->setProperty("ExtrapolationType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit62));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit62));

//                 part->dynamicCall("Update()");

//                 hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                 QVariant::fromValue(ref12), 0);

//                 if (split2.size() == 0)
//                 {
//                     break;
//                 }

//                 if (k == 0)
//                 {
//                     hybridShapeIntersection34 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                                    QVariant::fromValue(hybridShapeSplit52), QVariant::fromValue(split2[k+1]));
//                     hybridShapeIntersection34->setProperty("PointType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection34));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection34));
//                     part->dynamicCall("Update()");

//                     selection->dynamicCall("Clear()");
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection34));
//                     selection->dynamicCall("Delete()");

//                     if (hybridShapeIntersection34)
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit52));
//                     }
//                     else
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit62));
//                     }
//                 }
//                 if (k > 0)
//                 {
//                     hybridShapeIntersection35 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                                    QVariant::fromValue(hybridShapeSplit52), QVariant::fromValue(split2[k-1]));
//                     hybridShapeIntersection35->setProperty("PointType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection35));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection35));
//                     part->dynamicCall("Update()");

//                     selection->dynamicCall("Clear()");
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection35));
//                     selection->dynamicCall("Delete()");

//                     if (hybridShapeIntersection35)
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit62));

//                         hybridShapeSplit72 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit52), QVariant::fromValue(split2[k-1]), -1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit52), 0);
//                         hybridShapeSplit72->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit72));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit72));

//                         hybridShapeSplit82 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit52), QVariant::fromValue(split2[k-1]), 1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit52), 0);
//                         hybridShapeSplit82->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit82));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit82));

//                         part->dynamicCall("Update()");
//                     }
//                     else
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit52));

//                         hybridShapeSplit92 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit62), QVariant::fromValue(split2[k-1]), -1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit62), 0);
//                         hybridShapeSplit92->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit92));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit92));

//                         hybridShapeSplit102 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                  QVariant::fromValue(hybridShapeSplit62), QVariant::fromValue(split2[k-1]), 1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit62), 0);
//                         hybridShapeSplit102->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit102));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit102));

//                         part->dynamicCall("Update()");
//                     }
//                 }
//             }
//         }

//         if (j == split1.size() - 1)
//         {
//             hybridShapeAssemble4 = hybridShapeFactory->querySubObject("AddNewJoin(QVariant, QVariant)",
//                                                                       QVariant::fromValue(ref1), QVariant::fromValue(split1[j]));

//             hybridShapeAssemble4->setProperty("SetConnex", 1);
//             hybridShapeAssemble4->setProperty("SetManifold", 1);
//             hybridShapeAssemble4->setProperty("SetSimplify", 0);
//             hybridShapeAssemble4->setProperty("SetSuppressMode", 0);
//             hybridShapeAssemble4->setProperty("SetDeviation", 0.001);
//             hybridShapeAssemble4->setProperty("SetAngularToleranceMode", 0);
//             hybridShapeAssemble4->setProperty("SetAngularTolerance", 0.5);
//             hybridShapeAssemble4->setProperty("SetFederationPropagation", 0);

//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeAssemble4));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeAssemble4));
//             part->dynamicCall("Update()");

//             ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                          QVariant::fromValue(hybridShapeAssemble4));

//             for (int k = 0; k < split2.size(); k++)
//             {
//                 hybridShapeSplit53 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                         QVariant::fromValue(ref12), QVariant::fromValue(split2[k]), 1);
//                 hybridShapeSplit53->setProperty("ExtrapolationType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit53));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit53));

//                 hybridShapeSplit63 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                         QVariant::fromValue(ref12), QVariant::fromValue(split2[k]), -1);
//                 hybridShapeSplit63->setProperty("ExtrapolationType", 1);
//                 hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit63));
//                 part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit63));

//                 part->dynamicCall("Update()");

//                 hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                 QVariant::fromValue(ref12), 0);

//                 if (split2.size() == 0)
//                 {
//                     break;
//                 }

//                 if (k == 0)
//                 {
//                     hybridShapeIntersection36 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                                    QVariant::fromValue(hybridShapeSplit53), QVariant::fromValue(split2[k+1]));
//                     hybridShapeIntersection36->setProperty("PointType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection36));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection36));
//                     part->dynamicCall("Update()");

//                     selection->dynamicCall("Clear()");
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection36));
//                     selection->dynamicCall("Delete()");

//                     if (hybridShapeIntersection36)
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit53));
//                     }
//                     else
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit53));
//                     }
//                 }
//                 if (k > 0)
//                 {
//                     hybridShapeIntersection37 = hybridShapeFactory->querySubObject("AddNewIntersection(QVariant, QVariant)",
//                                                                                    QVariant::fromValue(hybridShapeSplit53), QVariant::fromValue(split2[k-1]));
//                     hybridShapeIntersection37->setProperty("PointType", 1);
//                     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeIntersection37));
//                     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeIntersection37));
//                     part->dynamicCall("Update()");

//                     selection->dynamicCall("Clear()");
//                     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeIntersection37));
//                     selection->dynamicCall("Delete()");

//                     if (hybridShapeIntersection37)
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit63));

//                         hybridShapeSplit73 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit53), QVariant::fromValue(split2[k-1]), -1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit53), 0);
//                         hybridShapeSplit73->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit73));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit73));

//                         hybridShapeSplit83 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit53), QVariant::fromValue(split2[k-1]), 1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit53), 0);
//                         hybridShapeSplit83->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit83));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit83));

//                         part->dynamicCall("Update()");
//                     }
//                     else
//                     {
//                         ref12 = part->querySubObject("CreateReferenceFromObject(QVariant)",
//                                                      QVariant::fromValue(hybridShapeSplit53));

//                         hybridShapeSplit93 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                 QVariant::fromValue(hybridShapeSplit63), QVariant::fromValue(split2[k-1]), -1);
//                         hybridShapeSplit93->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit63), 0);
//                         hybridShapeSplit92->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit93));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit93));

//                         hybridShapeSplit103 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                                                  QVariant::fromValue(hybridShapeSplit63), QVariant::fromValue(split2[k-1]), 1);
//                         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)",
//                                                         QVariant::fromValue(hybridShapeSplit63), 0);
//                         hybridShapeSplit103->setProperty("ExtrapolationType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit103));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit103));

//                         part->dynamicCall("Update()");
//                     }
//                 }
//             }
//         }
//     }
// }
// }


// 选择闭合边界 --------------------------------------------------------
// void MainWindow::on_pushButton_10_clicked()
// {
//     partDocument = catia->querySubObject("ActiveDocument");

//     part = partDocument->querySubObject("Part");

//     hybridShapeFactory = part->querySubObject("hybridShapeFactory");

//     hybridBodies = part->querySubObject("hybridBodies");

//     redirector->log("请选择闭合路径，按ESC取消!");
//     QMessageBox::information(this, "信息", "请选择闭合路径，按ESC取消!");

//     hybridBody2 = hybridBodies->querySubObject("Add()");

//     selection = partDocument->querySubObject("Selection");

//     selection->dynamicCall("Clear()");

//     QVariantList filter;
//     filter << "BiDimFeatEdge" << "MonoDimInfinite";

//     result = selection->dynamicCall("SelectElement3(QVariant, QString, bool, int, bool)",
//                                     QVariant(filter), "请选择闭合路径", true, 2, false);

//     status = result.toString();
//     if (status == "Cancel" || status == "Redo")
//     {
//         return;
//     }
//     else if(status != "Redo")
//     {
//         count = selection->property("Count").toInt();
//         ui->lineEdit_6->setText(QString("%1条闭合路径").arg(count));

//         edge.clear();
//         for (int i = 1; i <= count; i++)
//         {
//             item = selection->querySubObject("Item(int)", i);
//             value = item->querySubObject("Value");
//             edge.append(value);
//         }
//     }

//     selection->dynamicCall("Clear()");

//     // 提取边线
//     hybridShapeExtractref.resize(edge.size());

//     for (int i = 1; i <= edge.size(); i++)
//     {
//         hybridShapeExtract = hybridShapeFactory->querySubObject("AddNewExtract(QVariant)", QVariant::fromValue(edge[i-1]));

//         hybridShapeExtract->setProperty("PropagationType", 3);
//         hybridShapeExtract->setProperty("ComplementaryExtract", false);
//         hybridShapeExtract->setProperty("IsFederated", false);

//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeExtract));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeExtract));
//         part->dynamicCall("Update()");

//         hybridShapeExtractref[i - 1] = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapeExtract));
//     }

//     // 生成中心点
//     hybridShapePointOnCurve1 = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                                             QVariant::fromValue(hybridShapeExtractref[0]), 0.5, false);
//     hybridShapePointOnCurve2 = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                                             QVariant::fromValue(hybridShapeExtractref[1]), 0.5, false);
//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurve1));
//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurve2));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurve1));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurve2));
//     part->dynamicCall("Update()");

//     hybridShapePointOnCurveRef1 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapePointOnCurve1));
//     hybridShapePointOnCurveRef2 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapePointOnCurve2));

//     // 接合曲面边界线
//     hybridShapeAssemble = hybridShapeFactory->querySubObject("AddNewJoin(QVariant, QVariant)", QVariant::fromValue(hybridShapeExtractref[0]), QVariant::fromValue(hybridShapeExtractref[1]));

//     for (int i = 2; i <= edge.size(); i++ )
//     {
//         hybridShapeAssemble->dynamicCall("AddElement(QVariant)", QVariant::fromValue(hybridShapeExtractref[i-1]));
//     }

//     hybridShapeAssemble->setProperty("SetConnex", 1);
//     hybridShapeAssemble->setProperty("SetManifold", 1);
//     hybridShapeAssemble->setProperty("SetSimplify", 0);
//     hybridShapeAssemble->setProperty("SetSuppressMode", 0);
//     hybridShapeAssemble->setProperty("SetDeviation", 0.001);
//     hybridShapeAssemble->setProperty("SetAngularToleranceMode", 0);
//     hybridShapeAssemble->setProperty("SetAngularTolerance", 0.5);
//     hybridShapeAssemble->setProperty("SetFederationPropagation", 0);

//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeAssemble));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeAssemble));
//     part->dynamicCall("Update()");

//     hybridShapeAssemble2 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapeAssemble));

//     selection->dynamicCall("Clear()");
//     visPropertySet = selection->querySubObject("VisProperties");
//     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeAssemble));
//     visPropertySet = visPropertySet->querySubObject("Parent");
//     visPropertySet->setProperty("SetShow", 1);
//     selection->dynamicCall("Clear()");

//     redirector->log("请选择扫描面，按ESC取消!");
//     QMessageBox::information(this, "信息", "请选择扫描面，按ESC取消!");

//     selection->dynamicCall("Clear()");

//     QVariantList filter1;
//     filter1 << "BiDimInfinite";

//     result = selection->dynamicCall("SelectElement2(QVariant, QString, bool)",
//                                     QVariant(filter1), "请选择扫描面", false);

//     status = result.toString();
//     if (status == "Cancel" || status == "Undo")
//     {
//         return;
//     }

//     if (status != "Redo")
//     {
//         item = selection->querySubObject("Item(int)", 1);
//         hybridShapeAssemble1 = item->querySubObject("Value");
//     }

//     selection->dynamicCall("Clear()");
// }


// 选择起始路径 --------------------------------------------------------
// void MainWindow::on_pushButton_11_clicked()
// {
//     partDocument = catia->querySubObject("ActiveDocument");

//     part = partDocument->querySubObject("Part");

//     hybridShapeFactory = part->querySubObject("hybridShapeFactory");

//     selection = partDocument->querySubObject("Selection");

//     redirector->log("请选择起始路径，按ESC取消!");
//     QMessageBox::information(this, "信息", "请选择起始路径，按ESC取消!");

//     selection->dynamicCall("Clear()");

//     QVariantList filter;
//     filter << "MonoDimInfinite";

//     result = selection->dynamicCall("SelectElement2(QVariant, QString, bool)",
//                                     QVariant(filter), "请选择起始路径", false);

//     status = result.toString();
//     if (status == "Cancel" || status == "Undo")
//     {
//         return;
//     }

//     if (status != "Redo")
//     {
//         item = selection->querySubObject("Item(int)", 1);
//         hybridShape = item->querySubObject("Value");

//         QString shapeName = hybridShape->property("Name").toString();
//         ui->lineEdit_7->setText(shapeName);
//     }

//     selection->dynamicCall("Clear()");

//     hybridShapeExtractref3 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShape));
// }


// 创建路径 --------------------------------------------------------
// void MainWindow::on_pushButton_12_clicked()
// {
//     qDebug() << "用户点击创建路径";

//     ui->progressBar->setValue(0);
//     windows = catia->querySubObject("Windows");

//     count = windows->property("Count").toInt();

//     activedoc.reserve(count);

//     for (int i = 1; i <= count; ++i)
//     {
//         mywindow = windows->querySubObject("Item(int)", i);
//         if (!mywindow) continue;

//         mywindow->dynamicCall("Activate()");
//         QThread::sleep(1); // 等待窗口切换完成

//         doc = catia->querySubObject("ActiveDocument");
//         activedoc.append(doc);
//     }

//     part = activedoc[0]->querySubObject("Part");

//     hybridShapeFactory = part->querySubObject("hybridShapeFactory");

//     selection = activedoc[0]->querySubObject("Selection");

//     // 检测分割曲面方向
//     qDebug() << "检测分割曲面方向";
//     hybridShapeSplit1 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//             QVariant::fromValue(hybridShapeAssemble1), QVariant::fromValue(hybridShapeAssemble2), hybridShapeSplitdir);
//     hybridShapeSplit1->setProperty("ExtrapolationType", 1);

//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit1));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeSplit1));
//     part->dynamicCall("Update()");

//     hybridShapeLinePtPttest = hybridShapeFactory->querySubObject("AddNewLinePtPtOnSupport(QVariant, QVariant, QVariant)",
//         QVariant::fromValue(hybridShapePointOnCurveRef1), QVariant::fromValue(hybridShapePointOnCurveRef2), QVariant::fromValue(hybridShapeSplit1));

//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeLinePtPttest));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeLinePtPttest));

//     hybridShapePointOnCurvetest = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                                           QVariant::fromValue(hybridShapeLinePtPttest), 0.5, false);
//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurvetest));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurvetest));
//     part->dynamicCall("Update()");

//     test0 = part->dynamicCall("IsUpToDate(QVariant)", QVariant::fromValue(hybridShapePointOnCurvetest)).toBool();

//     if (test0 == false)
//     {
//         hybridShapeSplitdir = 1;
//     }
//     selection->dynamicCall("Clear()");
//     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapePointOnCurvetest));
//     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeLinePtPttest));
//     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeSplit1));
//     selection->dynamicCall("Delete()");

//     hybridShapeSplit2 = hybridShapeFactory->querySubObject("AddNewHybridSplit(QVariant, QVariant, int)",
//                                                            QVariant::fromValue(hybridShapeAssemble1), QVariant::fromValue(hybridShapeAssemble2), hybridShapeSplitdir);
//     hybridShapeSplit2->setProperty("ExtrapolationType", 1);

//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeSplit2));
//     part->dynamicCall("SetInWorkObject(QVariant)", QVariant::fromValue(hybridShapeSplit2));
//     part->dynamicCall("Update()");

//     reference2 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapeSplit2));

//     // 定义机器人与工件
//     qDebug() << "定义机器人与工件";
//     pprdocument = activedoc[1]->querySubObject("PPRDocument");

//     product = pprdocument->querySubObject("Products");

//     selection = pprdocument->querySubObject("Selection");

//     redirector->log("请选择工件，按ESC取消!");
//     QMessageBox::information(this, "信息", "请选择工件，按ESC取消!");

//     QVariantList filter;
//     filter << "Product";

//     selection->dynamicCall("Clear()");

//     result = selection->dynamicCall("SelectElement2(QVariant, QString, bool)", QVariant(filter), "请选择工件", false);

//     status = result.toString();
//     if (status == "Cancel" || status == "Undo")
//     {
//         return;
//     }

//     if (status != "Redo")
//     {
//         item = selection->querySubObject("Item(int)", 1);
//         work = item->querySubObject("Value");
//     }

//     selection->dynamicCall("Clear()");

//     redirector->log("请选择机器人，按ESC取消!");
//     QMessageBox::information(this, "信息", "请选择机器人，按ESC取消!");

//     result = selection->dynamicCall("SelectElement2(QVariant, QString, bool)", QVariant(filter), "请选择机器人", false);

//     status = result.toString();
//     if (status == "Cancel" || status == "Undo")
//     {
//         return;
//     }

//     if (status != "Redo")
//     {
//         item = selection->querySubObject("Item(int)", 1);
//         objRobot = item->querySubObject("Value");
//     }

//     selection->dynamicCall("Clear()");

//     longmen = pprdocument->querySubObject("Resources")->querySubObject("Item(QString)", "Longmen.1");

//     // 创建机器人任务
//     qDebug() << "创建机器人任务";
//     objRobotTask = new QAxObject();
//     objRobotTaskFactory = objRobot->querySubObject("GetTechnologicalObject(QString)", "RobotTaskFactory");
//     objRobotTaskFactory->dynamicCall("CreateRobotTask(QString, QVariant)", "RobotTask1", QVariant::fromValue(objRobotTask));

//     // 检测平行曲线方向
//     qDebug() << "检测平行曲线方向";
//     hybridShapeCurvePartest = hybridShapeFactory->querySubObject("AddNewCurvePar(QVariant, QVariant, double, bool, bool)",
//         QVariant::fromValue(hybridShapeExtractref3), QVariant::fromValue(reference2), 10.0, linedir, false);
//     hybridShapeCurvePartest->setProperty("SmoothingType", 0);
//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeCurvePartest));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeCurvePartest));

//     hybridShapePointOnCurvetestbegin = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                             QVariant::fromValue(hybridShapeCurvePartest), 0.0, false);
//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurvetestbegin));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurvetestbegin));

//     part->dynamicCall("Update()");

//     test = part->dynamicCall("IsUpToDate(QVariant)", QVariant::fromValue(hybridShapePointOnCurvetestbegin)).toBool();

//     if (test == false) { linedir = false; }

//     selection->dynamicCall("Clear()");
//     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeCurvePartest));
//     selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapePointOnCurvetestbegin));
//     selection->dynamicCall("Delete()");

//     // 平移起始路径
//     qDebug() << "平移起始路径";
//     hybridShapeCurvePar1 = hybridShapeFactory->querySubObject("AddNewCurvePar(QVariant, QVariant, double, bool, bool)",
//                                                                  QVariant::fromValue(hybridShapeExtractref3), QVariant::fromValue(reference2), 0.0, linedir, false);
//     hybridShapeCurvePar1->setProperty("SmoothingType", 0);
//     hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeCurvePar1));
//     part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeCurvePar1));

//     reference1 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapeCurvePar1));

//     // 添加路径线
//     qDebug() << "添加路径线";
//     for (int i = 0; i < ui->lineEdit_9->text().toInt(); i++)
//     {
//         // 平移后曲线添加起始点
//         qDebug() << "平移后曲线添加起始点";
//         hybridShapePointOnCurvebegin = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                                                           QVariant::fromValue(reference1), 0.0, false);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurvebegin));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurvebegin));
//         part->dynamicCall("Update()");

//         continue1 = part->dynamicCall("IsUpToDate(QVariant)", QVariant::fromValue(hybridShapePointOnCurvebegin)).toBool();

//         if (continue1 == false)
//         {
//             selection->dynamicCall("Clear()");
//             selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeCurvePar2));
//             selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapePointOnCurvebegin));
//             selection->dynamicCall("Delete()");
//             return;
//         }

//         // 平移后曲线以起始点为边界外插延伸至曲面边界
//         qDebug() << "平移后曲线以起始点为边界外插延伸至曲面边界";
//         hybridShapeExtrapol1 = hybridShapeFactory->querySubObject("AddNewExtrapolUntil(QVariant, QVariant, QVariant)",
//                                                                   QVariant::fromValue(hybridShapePointOnCurvebegin), QVariant::fromValue(reference1), QVariant::fromValue(hybridShapeAssemble2));

//         hybridShapeExtrapol1->dynamicCall("BorderType", 1);
//         hybridShapeExtrapol1->setProperty("Support(QVariant)", QVariant::fromValue(reference2));
//         hybridShapeExtrapol1->dynamicCall("SetAssemble(bool)", true);
//         hybridShapeExtrapol1->dynamicCall("PropagationMode", 0);
//         hybridShapeExtrapol1->dynamicCall("ExtendEdgesMode", false);
//         hybridShapeExtrapol1->dynamicCall("ConstantLengthMode", false);
//         hybridShapeExtrapol1->dynamicCall("SetContinuityType(int, int)", 1, 0);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeExtrapol1));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeExtrapol1));

//         // 平移后曲线添加终止点
//         qDebug() << "平移后曲线添加终止点";
//         hybridShapePointOnCurveend = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                                                         QVariant::fromValue(hybridShapeExtrapol1), 1.0, false);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurveend));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurveend));

//         hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)", QVariant::fromValue(hybridShapePointOnCurveend), 0);

//         // 平移后曲线以终止点为边界外插延伸至曲面边界
//         qDebug() << "平移后曲线以终止点为边界外插延伸至曲面边界";
//         hybridShapeExtrapol2 = hybridShapeFactory->querySubObject("AddNewExtrapolUntil(QVariant, QVariant, QVariant)",
//                                                                   QVariant::fromValue(hybridShapePointOnCurveend), QVariant::fromValue(hybridShapeExtrapol1), QVariant::fromValue(hybridShapeAssemble2));

//         hybridShapeExtrapol2->dynamicCall("BorderType", 1);
//         hybridShapeExtrapol2->setProperty("Support(QVariant)", QVariant::fromValue(reference2));
//         hybridShapeExtrapol2->dynamicCall("SetAssemble(bool)", true);
//         hybridShapeExtrapol2->dynamicCall("PropagationMode", 0);
//         hybridShapeExtrapol2->dynamicCall("ExtendEdgesMode", false);
//         hybridShapeExtrapol2->dynamicCall("ConstantLengthMode", false);
//         hybridShapeExtrapol2->dynamicCall("SetContinuityType(int, int)", 1, 0);

//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeExtrapol2));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeExtrapol2));

//         // 外插延伸后曲线添加起始点与终止点
//         qDebug() << "外插延伸后曲线添加起始点与终止点";
//         hybridShapePointOnCurve2 = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                                                       QVariant::fromValue(hybridShapeExtrapol2), 0.0, false);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurve2));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurve2));

//         refstartpoint = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapePointOnCurve2));

//         hybridShapePointOnCurve3 = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromPercent(QVariant, double, bool)",
//                                                                       QVariant::fromValue(hybridShapeExtrapol2), 1.0, false);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurve3));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurve3));
//         part->dynamicCall("Update()");

//         refbeginpoint = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapePointOnCurve3));

//         // 测量曲线长度
//         qDebug() << "测量曲线长度";
//         TheSPAWorkbench1 = catia->querySubObject("ActiveDocument")->querySubObject("GetWorkbench(QString)", "SPAWorkbench");
//         TheMeasurable1 = TheSPAWorkbench1->querySubObject("GetMeasurable(QVariant)", QVariant::fromValue(hybridShapeExtrapol2));
//         TheMeasurable5 = TheSPAWorkbench1->querySubObject("GetMeasurable(QVariant)", QVariant::fromValue(hybridShapeAssemble2));

//         area = TheMeasurable5->property("Length").toDouble();
//         l = TheMeasurable1->property("Length").toDouble();
//         ui->progressBar->setValue(i*100.0/((area/2.0-l)/ui->lineEdit_8->text().toInt()) + 3);

//         lenth = ui->lineEdit_10->text().toDouble();

//         refpoint.resize(j + 1);

//         hybridShapePointOnCurve1 = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromDistance(QVariant, double, bool)",
//                                                                       QVariant::fromValue(hybridShapeExtrapol2), 0.0, pointdir);

//         refpoint[0] = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapePointOnCurve1));

//         hybridShapePointOnCurve1->setProperty("DistanceType", 1);

//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurve1));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurve1));

//         // 添加曲线上点
//         qDebug() << "添加曲线上点";
//         while (lenth <= l)
//         {
//             // 创建指定距离处的点
//             qDebug() << "创建指定距离处的点";
//             hybridShapePointOnCurve4 = hybridShapeFactory->querySubObject("AddNewPointOnCurveFromDistance(QVariant, double, bool)",
//                                                                           QVariant::fromValue(hybridShapeExtrapol2), lenth + 0.0, pointdir);
//             refpoint[j] = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapePointOnCurve4));
//             hybridShapePointOnCurve4->setProperty("DistanceType", 1);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurve4));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurve4));

//             hybridShapeFactory->dynamicCall("GSMVisibility(QVariant, int)", QVariant::fromValue(hybridShapePointOnCurve4), 0);

//             // 添加相邻两点切线
//             qDebug() << "添加相邻两点切线";
//             hybridShapeLineTangency1 = hybridShapeFactory->querySubObject("AddNewLineTangency(QVariant, QVariant, double, double, bool)",
//                                                                           QVariant::fromValue(hybridShapeExtrapol2), QVariant::fromValue(refpoint[j - 1]), 0.0, 20.0, false);
//             hybridShapeLineTangency2 = hybridShapeFactory->querySubObject("AddNewLineTangency(QVariant, QVariant, double, double, bool)",
//                                                                           QVariant::fromValue(hybridShapeExtrapol2), QVariant::fromValue(refpoint[j]), 0.0, 20.0, false);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeLineTangency1));
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeLineTangency2));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeLineTangency1));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeLineTangency2));
//             part->dynamicCall("Update()");

//             // 测量切线方向
//             qDebug() << "测量切线方向";
//             Direction1.resize(3);
//             Direction2.resize(3);
//             QVariant DirectionVar1 = QVariant::fromValue(Direction1);
//             QVariant DirectionVar2 = QVariant::fromValue(Direction2);
//             TheSPAWorkbench2 = catia->querySubObject("ActiveDocument")->querySubObject("GetWorkbench(QString)", "SPAWorkbench");
//             TheMeasurable2 = TheSPAWorkbench2->querySubObject("GetMeasurable(QVariant)", QVariant::fromValue(hybridShapeLineTangency1));
//             TheMeasurable3 = TheSPAWorkbench2->querySubObject("GetMeasurable(QVariant)", QVariant::fromValue(hybridShapeLineTangency2));
//             TheMeasurable2->dynamicCall("GetDirection(QVariant)", DirectionVar1);
//             TheMeasurable3->dynamicCall("GetDirection(QVariant)", DirectionVar2);
//             QVariantList list1 = DirectionVar1.toList();
//             QVector3D vec1(list1[0].toDouble(), list1[1].toDouble(), list1[2].toDouble());
//             qDebug() << "Direction1:" << vec1;
//             QVariantList list2 = DirectionVar2.toList();
//             QVector3D vec2(list2[0].toDouble(), list2[1].toDouble(), list2[2].toDouble());
//             qDebug() << "Direction2:" << vec2;

//             // 计算切线夹角
//             qDebug() << "计算切线夹角";
//             // Angle = qAcos((Direction1[0].toDouble()*Direction2[0].toDouble() + Direction1[1].toDouble()*Direction2[1].toDouble() + Direction1[2].toDouble()*Direction2[2].toDouble()) / (qSqrt(qPow(Direction1[0].toDouble(),2) + qPow(Direction1[1].toDouble(),2) + qPow(Direction1[2].toDouble(),2)) * qSqrt(qPow(Direction2[0].toDouble(),2) + qPow(Direction2[1].toDouble(),2) + qPow(Direction2[2].toDouble(),2))));

//             // 曲率大则缩短步长
//             qDebug() << "曲率大则缩短步长";
//             if (Angle < 1.0)
//             {   // 两点间路径为直线则检测下一个点
//                 qDebug() << "两点间路径为直线则检测下一个点";
//                 if (H == true) { // 检测是否进行回退操作
//                     lastlenth = lenth;
//                     lenth += ui->lineEdit_10->text().toDouble(); // 回退后缩短检测步长
//                 } else {
//                     lastlenth = lenth;
//                     lenth += ui->lineEdit_11->text().toDouble(); // 直线阶段检测步长
//                     G = 1;
//                 }
//                 selection->dynamicCall("Clear()");
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeLineTangency1));
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeLineTangency2));
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapePointOnCurve4));
//                 selection->dynamicCall("Delete()");
//             }
//             else
//             {
//                 selection->dynamicCall("Clear()");
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeLineTangency1));
//                 selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapeLineTangency2));
//                 selection->dynamicCall("Delete()");

//                 switch (G)
//                 {
//                     case 0:
//                         lastlenth = lenth;
//                         lenth += 10.0 * qSqrt(ui->lineEdit_10->text().toDouble() / (qLn(Angle) / qLn(2.0) + 1.0));
//                         j++;
//                         H = false;
//                         refpoint.resize(j + 1);
//                         break;

//                     case 1:
//                         lenth = lastlenth + 10.0 * qSqrt(ui->lineEdit_10->text().toDouble() / (qLn(Angle) / qLn(2.0) + 1.0));
//                         G++;
//                         H = true;
//                         selection->dynamicCall("Clear()");
//                         selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapePointOnCurve4));
//                         selection->dynamicCall("Delete()");
//                         break;

//                     case 2:
//                         selection->dynamicCall("Clear()");
//                         selection->dynamicCall("Add(QVariant)", QVariant::fromValue(hybridShapePointOnCurve4));
//                         selection->dynamicCall("Delete()");

//                         hybridShapePointOnCurve5 = hybridShapeFactory->querySubObject(
//                             "AddNewPointOnCurveFromDistance(QVariant, double, bool)",
//                             QVariant::fromValue(hybridShapeExtrapol2), lastlenth, pointdir);

//                         refpoint[j] = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapePointOnCurve5));
//                         hybridShapePointOnCurve5->setProperty("DistanceType", 1);
//                         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapePointOnCurve5));
//                         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapePointOnCurve5));

//                         j++;
//                         refpoint.resize(j + 1);
//                         G = 0;
//                         break;
//                 }
//             }
//         }

//         // 设置向量大小
//         qDebug() << "设置向量大小";
//         QVector<QVector<double>*> allVectors =
//         {
//             &pointx, &pointy, &pointz, // 坐标数组
//             &tagx, &tagy, &tagz, // 标签点坐标
//             &normalx, &normaly, &normalz, // 法向量
//             &zaxisx, &zaxisy, &zaxisz, // 坐标系 Z 轴
//             &xaxisx, &xaxisy, &xaxisz, // 坐标系 X 轴
//             &yaxisx, &yaxisy, &yaxisz, // 坐标系 Y 轴
//             &R, &P, &y // 其他数组
//         };
//         for (auto vec : allVectors) {
//             vec->resize(refpoint.size());
//         }

//         // 交换点生成方向
//         qDebug() << "交换点生成方向";
//         if (pointdir == true)
//         {
//             pointdir = false;
//             startpoint = refbeginpoint;
//             endpoint = refstartpoint;
//         }
//         else
//         {
//             pointdir = true;
//             startpoint = refstartpoint;
//             endpoint = refbeginpoint;
//         }

//         refpoint[refpoint.size() - 1] = endpoint;

//         // 添加当前点法线
//         qDebug() << "添加当前点法线";
//         for (int j = 0; j < refpoint.size(); ++j)
//         {
//             HybridShapeLineNormal1 = hybridShapeFactory->querySubObject("AddNewLineNormal(QVariant, QVariant, int, int, bool)",
//                                                                         QVariant::fromValue(reference2), QVariant::fromValue(refpoint[j]), 0, 1, true);
//             hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(HybridShapeLineNormal1));
//             part->dynamicCall("SetInWorkObject", QVariant::fromValue(HybridShapeLineNormal1));
//             reference4 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(HybridShapeLineNormal1));
//             part->dynamicCall("Update()");

//             // 提取曲线上点坐标
//             qDebug() << "提取曲线上点坐标";
//             QVariantList Coordinates;
//             Coordinates << 0.0 << 0.0 << 0.0;
//             TheSPAWorkbench2 = catia->querySubObject("ActiveDocument")->querySubObject("GetWorkbench(QString)", "SPAWorkbench");
//             TheMeasurable2 = TheSPAWorkbench2->querySubObject("GetMeasurable(QVariant)", QVariant::fromValue(refpoint[j]));
//             TheMeasurable2->dynamicCall("GetPoint(QVariant)", QVariant(Coordinates));
//             qDebug() << Coordinates;
//             pointx[j] = Coordinates[0].toDouble();
//             pointy[j] = Coordinates[1].toDouble();
//             pointz[j] = Coordinates[2].toDouble();

//             // 提取点法线向量
//             qDebug() << "提取点法线向量";
//             Direction.resize(3);
//             Direction << 0.0 << 0.0 << 0.0;
//             TheMeasurable3 = TheSPAWorkbench2->querySubObject("GetMeasurable(QVariant)", QVariant::fromValue(reference4));
//             TheMeasurable3->dynamicCall("GetDirection(QVariant)", QVariant::fromValue(Direction));

//             normalx[j] = Direction[0].toDouble();
//             normaly[j] = Direction[1].toDouble();
//             normalz[j] = Direction[2].toDouble();

//             selection->dynamicCall("Clear()");
//             selection->dynamicCall("Add(QVariant)", QVariant::fromValue(HybridShapeLineNormal1));
//             selection->dynamicCall("Delete()");
//         }

//         // 提取曲线起点坐标
//         qDebug() << "提取曲线起点坐标";
//         beginpoint.resize(3);
//         TheMeasurable4 = TheSPAWorkbench2->querySubObject("GetMeasurable(QVariant)", QVariant::fromValue(startpoint));
//         TheMeasurable4->dynamicCall("GetPoint(QVariant)", QVariant::fromValue(beginpoint));

//         bp.resize(3);
//         bp[0] = beginpoint[0];
//         bp[1] = beginpoint[1];
//         bp[2] = beginpoint[2];

//         // 平移外插延伸后曲线
//         qDebug() << "平移外插延伸后曲线";
//         hybridShapeCurvePar2 = hybridShapeFactory->querySubObject("AddNewCurvePar(QVariant, QVariant, double, bool, bool)",
//                                                                   QVariant::fromValue(hybridShapeExtrapol2), QVariant::fromValue(reference2), ui->lineEdit_8->text().toDouble() + 0.0, linedir, false);
//         hybridShapeCurvePar2->setProperty("SmoothingType", 0);
//         hybridBody2->dynamicCall("AppendHybridShape(QVariant)", QVariant::fromValue(hybridShapeCurvePar2));
//         part->dynamicCall("SetInWorkObject", QVariant::fromValue(hybridShapeCurvePar2));

//         reference1 = part->querySubObject("CreateReferenceFromObject(QVariant)", QVariant::fromValue(hybridShapeCurvePar2));

//         // 读取product到process的旋转矩阵
//         qDebug() << "读取product到process的旋转矩阵";
//         oAxisComponentsArrayprocess.resize(12);
//         work->querySubObject("Position")->dynamicCall("GetComponents(QVariant)", QVariant::fromValue(oAxisComponentsArrayprocess));

//         // 添加TagGroup
//         qDebug() << "添加TagGroup";
//         objTagGroup = new QAxObject();
//         objTagGroupFactory = work->querySubObject("GetTechnologicalObject(QString)", "TagGroupFactory");
//         objTagGroupFactory->dynamicCall("CreateTagGroup(QString, bool, QVariant, QVariant)", "MyGroup", true, QVariant::fromValue(work), QVariant::fromValue(objTagGroup));

//         // 创建Tag点
//         qDebug() << "创建Tag点";
//         for (int j = 0; j < refpoint.size(); ++j)
//         {
//             // 点的product坐标系到process坐标系变换
//             qDebug() << "点的product坐标系到process坐标系变换";
//             tagx[j] = pointx[j] * oAxisComponentsArrayprocess[0].toDouble() +
//                       pointy[j] * oAxisComponentsArrayprocess[3].toDouble() +
//                       pointz[j] * oAxisComponentsArrayprocess[6].toDouble() +
//                       oAxisComponentsArrayprocess[9].toDouble();

//             tagy[j] = pointx[j] * oAxisComponentsArrayprocess[1].toDouble() +
//                       pointy[j] * oAxisComponentsArrayprocess[4].toDouble() +
//                       pointz[j] * oAxisComponentsArrayprocess[7].toDouble() +
//                       oAxisComponentsArrayprocess[10].toDouble();

//             tagz[j] = pointx[j] * oAxisComponentsArrayprocess[2].toDouble() +
//                       pointy[j] * oAxisComponentsArrayprocess[5].toDouble() +
//                       pointz[j] * oAxisComponentsArrayprocess[8].toDouble() +
//                       oAxisComponentsArrayprocess[11].toDouble();

//             // 计算Z轴向量
//             qDebug() << "计算Z轴向量";
//             zaxisx[j] = -(normalx[j] * oAxisComponentsArrayprocess[0].toDouble() +
//                           normaly[j] * oAxisComponentsArrayprocess[3].toDouble() +
//                           normalz[j] * oAxisComponentsArrayprocess[6].toDouble());

//             zaxisy[j] = -(normalx[j] * oAxisComponentsArrayprocess[1].toDouble() +
//                           normaly[j] * oAxisComponentsArrayprocess[4].toDouble() +
//                           normalz[j] * oAxisComponentsArrayprocess[7].toDouble());

//             zaxisz[j] = -(normalx[j] * oAxisComponentsArrayprocess[2].toDouble() +
//                           normaly[j] * oAxisComponentsArrayprocess[5].toDouble() +
//                           normalz[j] * oAxisComponentsArrayprocess[8].toDouble());
//             t0 = std::sqrt( zaxisx[j] * zaxisx[j] +
//                             zaxisy[j] * zaxisy[j] +
//                             zaxisz[j] * zaxisz[j] );
//             zaxisx[j] /= t0;
//             zaxisy[j] /= t0;
//             zaxisz[j] /= t0;

//             // 计算X轴向量
//             qDebug() << "计算X轴向量";
//             xx = -1;
//             yy = 0;
//             zz = 0;

//             // X轴与Z轴正交化
//             qDebug() << "X轴与Z轴正交化";
//             d0 = (zaxisx[j] * xx + zaxisy[j] * yy + zaxisz[j] * zz) / (zaxisx[j] * zaxisx[j] + zaxisy[j] * zaxisy[j] + zaxisz[j] * zaxisz[j]);

//             xaxisx[j] = xx - d0 * zaxisx[j];
//             xaxisy[j] = yy - d0 * zaxisy[j];
//             xaxisz[j] = zz - d0 * zaxisz[j];

//             // 单位化
//             qDebug() << "单位化";
//             d2 = std::sqrt( xaxisx[j] * xaxisx[j] +
//                            xaxisy[j] * xaxisy[j] +
//                            xaxisz[j] * xaxisz[j] );
//             xaxisx[j] /= d2;
//             xaxisy[j] /= d2;
//             xaxisz[j] /= d2;

//             // 计算Y轴向量
//             qDebug() << "计算Y轴向量";
//             yaxisx[j] = xaxisy[j] * zaxisz[j] - zaxisy[j] * xaxisz[j];
//             yaxisy[j] = xaxisz[j] * zaxisx[j] - xaxisx[j] * zaxisz[j];
//             yaxisz[j] = xaxisx[j] * zaxisy[j] - xaxisy[j] * zaxisx[j];

//             t3 = std::sqrt( yaxisx[j] * yaxisx[j] +
//                            yaxisy[j] * yaxisy[j] +
//                            yaxisz[j] * yaxisz[j] );
//             yaxisx[j] /= t3;
//             yaxisy[j] /= t3;
//             yaxisz[j] /= t3;

//             // 计算RPY角
//             qDebug() << "计算RPY角";
//             P[j] = std::atan2(-xaxisz[j], std::sqrt(xaxisx[j] * xaxisx[j] + xaxisy[j] * xaxisy[j]));
//             R[j] = std::atan2(xaxisy[j] / std::cos(P[j]), xaxisx[j] / std::cos(P[j]));
//             y[j] = -std::atan2(yaxisz[j] / std::cos(P[j]), zaxisz[j] / std::cos(P[j]));
//         }

//         // for (int j = 0; j < refpoint.size(); ++j)
//         // {
//         //     // 创建operation
//         //     qDebug() << "创建operation";
//         //     objAfterOperation = objOperation;
//         //     objRobotTask->dynamicCall("CreateOperation(QVariant, QVariant, QVariant)",
//         //                                  QVariant::fromValue(objRefOperation), QVariant::fromValue(objAfterOperation), QVariant::fromValue(objOperation));

//         //     // 添加RobotMotion
//         //     qDebug() << "添加RobotMotion";
//         //     objOperation->dynamicCall("CreateRobotMotion(QVariant, bool, QVariant)", QVariant::fromValue(objRefAct), true, QVariant::fromValue(objRobotmotion));

//         //     // 创建tag点
//         //     qDebug() << "创建tag点";
//         //     objTagGroup->dynamicCall("CreateTag(QVariant)", QVariant::fromValue(objTag));
//         //     objTag->dynamicCall("SetName(QString)", QString("Tag%1").arg(j));
//         //     objTag->dynamicCall("SetType(QString)", "Manufacturing");
//         //     objTag->dynamicCall("SetXYZ(double, double, double)", tagx[j], tagy[j], tagz[j]);
//         //     objTag->dynamicCall("SetYPR(double, double, double)", y[j], P[j], R[j]);

//         //     // 添加tag点至RobotMotion中
//         //     qDebug() << "添加tag点至RobotMotion中";
//         //     objRobotmotion->dynamicCall("SetTagTarget(QVariant)", QVariant::fromValue(objTag));

//         //     // // 设置RobotMotion属性
//         //     qDebug() << "设置RobotMotion属性";
//         //     // objRobotmotion->setProperty("SetMotionType", 1); // 设置为JNT
//         //     // objRobotmotion->setProperty("SetConfig", 3); // 设置为4号解

//         //     // selection->dynamicCall("Clear()");

//         //     // visPropertySet1 = selection->querySubObject("VisProperties");

//         //     // selection->dynamicCall("Add(QVariant)", QVariant::fromValue(objTagGroup));

//         //     // visPropertySet1 = visPropertySet1->querySubObject("Parent");

//         //     // visPropertySet1->dynamicCall("SetShow(int)", 1);

//         //     // selection->dynamicCall("Clear()");

//         //     // objDevice = longmen->querySubObject("GetTechnologicalObject(QString)", "DOFState");

//         //     // QVariantList listOfDOFValues;
//         //     // objDevice->dynamicCall("GetDeviceDOFValues(QVariant)", QVariant::fromValue(listOfDOFValues));
//         // }
//     }

//     part->dynamicCall("Update()");

//     ui->progressBar->setValue(100);

//     QMessageBox::information(nullptr, "Info", "路径规划已完成，请进行仿真");

//     // Unload robot
// }
