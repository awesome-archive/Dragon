================
:mod:`Workspace`
================

.. toctree::
   :hidden:

Tensor
------

==============================    =============================================================================
List                              Brief
==============================    =============================================================================
`HasTensor`_                      Query whether tensor has registered in current workspace.
`CreateFiller`_                   Create the filler in the backend.
`GetTensorName`_                  Query the name represented in current workspace.
`SetTensorAlias`_                 Bind a alias to a existed tensor.
`FeedTensor`_                     Feed the values to the given tensor.
`FetchTensor`_                    Fetch the values of given tensor.
`ResetTensor`_                    Reset the memory of given tensor.
==============================    =============================================================================


Operator
--------

==============================    =============================================================================
List                              Brief
==============================    =============================================================================
`RunOperator`_                    Run the operator in the VM backend.
==============================    =============================================================================


Graph
-----

==============================    =============================================================================
List                              Brief
==============================    =============================================================================
`CreateGraph`_                    Create the graph in the backend.
`RunGraph`_                       Run the specific graph.
==============================    =============================================================================

Misc
----

==============================    =============================================================================
List                              Brief
==============================    =============================================================================
`Snapshot`_                       Snapshot tensors into a binary file.
`Restore`_                        Restore tensors from a binary file.
`SwitchWorkspace`_                Switch to the specific Workspace.
`MoveWorkspace`_                  Move the source workspace into the target workspace.
`ResetWorkspace`_                 Reset the specific workspace.
`ClearWorkspace`_                 Clear the specific workspace.
`LogMetaGraph`_                   Log the meta graph.
`ExportMetaGraph`_                Export the meta graph into a file under specific folder.
==============================    =============================================================================

API Reference
-------------

.. automodule:: dragon.core.workspace
    :members:
    :undoc-members:
    :show-inheritance:

.. _SwitchWorkspace: #dragon.core.workspace.SwitchWorkspace
.. _MoveWorkspace: #dragon.core.workspace.MoveWorkspace
.. _ResetWorkspace: #dragon.core.workspace.ResetWorkspace
.. _ClearWorkspace: #dragon.core.workspace.ClearWorkspace
.. _CreateGraph: #dragon.core.workspace.CreateGraph
.. _HasTensor: #dragon.core.workspace.HasTensor
.. _GetTensorName: #dragon.core.workspace.GetTensorName
.. _SetTensorAlias: #dragon.core.workspace.SetTensorAlias
.. _CreateFiller: #dragon.core.workspace.CreateFiller
.. _FetchTensor: #dragon.core.workspace.FetchTensor
.. _FeedTensor: #dragon.core.workspace.FeedTensor
.. _ResetTensor: #dragon.core.workspace.ResetTensor
.. _RunOperator: #dragon.core.workspace.RunOperator
.. _RunGraph: #dragon.core.workspace.RunGraph
.. _Snapshot: #dragon.core.workspace.Snapshot
.. _Restore: #dragon.core.workspace.Restore
.. _LogMetaGraph: #dragon.core.workspace.LogMetaGraph
.. _ExportMetaGraph: #dragon.core.workspace.ExportMetaGraph

.. _theano.function(*args, **kwargs): ../vm/theano/compile.html#dragon.vm.theano.compile.function.function
.. _config.ExportMetaGraph(prefix): ../config.html#dragon.config.ExportMetaGraph