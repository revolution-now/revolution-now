// This file is #include'd into a cpp module and compiled.

REQUEST( won_battle_europeans,
  DIM( tempo,     IN, fast, medium                             );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fife_and_drum, fiddle                    );
  DIM( sentiment, IN, happy, war_triumph                       );
  DIM( tonality,  IN, major                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( won_battle_natives,
  DIM( tempo,     IN, fast, medium                             );
  DIM( genre,     IN, trad, classical                          );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fife_and_drum, fiddle                    );
  DIM( sentiment, IN, happy, war_triumph                       );
  DIM( tonality,  IN, major                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( lost_battle_europeans,
  DIM( tempo,     IN, medium, slow                             );
  DIM( genre,     IN, trad, classical                          );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fife_and_drum, fiddle, orchestrated      );
  DIM( sentiment, IN, sad, war_lost                            );
  DIM( tonality,  IN, major, minor                             );
  DIM( purpose,   IN, standard, special_event                  );
)

REQUEST( lost_battle_natives,
  DIM( tempo,     IN, fast, medium                             );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, native                                   );
  DIM( inst,      IN, percussive                               );
  DIM( sentiment, IN, war_triumph                              );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( slow_sad,
  DIM( tempo,     IN, slow                                     );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, old_world                                );
  DIM( inst,      IN, fife_and_drum, fiddle                    );
  DIM( sentiment, IN, sad                                      );
  DIM( tonality,  IN, minor                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( happy_fast,
  DIM( tempo,     IN, fast                                     );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fiddle                                   );
  DIM( sentiment, IN, happy                                    );
  DIM( tonality,  IN, major                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( orchestrated,
  DIM( culture,   IN, new_world, old_world                     );
  DIM( inst,      IN, orchestrated                             );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( fiddle_tune,
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fiddle                                   );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( fife_drum_sad,
  DIM( tempo,     IN, medium, slow                             );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fife_and_drum                            );
  DIM( sentiment, IN, sad, war_lost                            );
  DIM( tonality,  IN, minor                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( fife_drum_slow,
  DIM( tempo,     IN, slow                                     );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fife_and_drum                            );
  DIM( sentiment, IN, happy, war_triumph                       );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( fife_drum_fast,
  DIM( tempo,     IN, fast                                     );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fife_and_drum                            );
  DIM( sentiment, IN, happy, war_triumph                       );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( fife_drum_happy,
  DIM( tempo,     IN, fast, medium                             );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, new_world                                );
  DIM( inst,      IN, fife_and_drum                            );
  DIM( sentiment, IN, happy                                    );
  DIM( tonality,  IN, major                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( native_sad,
  DIM( tempo,     IN, medium, slow                             );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, native                                   );
  DIM( inst,      IN, percussive                               );
  DIM( sentiment, IN, sad, war_lost                            );
  DIM( tonality,  IN, minor                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( native_happy,
  DIM( tempo,     IN, fast, medium                             );
  DIM( genre,     IN, trad                                     );
  DIM( culture,   IN, native                                   );
  DIM( inst,      IN, percussive                               );
  DIM( sentiment, IN, happy,                                   );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( king_happy,
  DIM( tempo,     IN, fast, medium                             );
  DIM( genre,     IN, classical                                );
  DIM( culture,   IN, old_world                                );
  DIM( inst,      IN, orchestrated                             );
  DIM( sentiment, IN, happy                                    );
  DIM( tonality,  IN, major                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( king_sad,
  DIM( tempo,     IN, medium, slow                             );
  DIM( genre,     IN, classical                                );
  DIM( culture,   IN, old_world                                );
  DIM( inst,      IN, orchestrated                             );
  DIM( sentiment, IN, sad                                      );
  DIM( tonality,  IN, minor                                    );
  DIM( purpose,   IN, standard                                 );
)

REQUEST( king_war,
  DIM( tempo,     IN, fast, medium                             );
  DIM( culture,   IN, old_world                                );
  DIM( inst,      IN, orchestrated                             );
  DIM( sentiment, IN, war_triumph                              );
  DIM( tonality,  IN, major                                    );
  DIM( purpose,   IN, standard                                 );
)
