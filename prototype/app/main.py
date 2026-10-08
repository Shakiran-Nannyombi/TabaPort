"""Render's saved start command is `uvicorn main:app`.

The hub is Flask. This module is the ASGI wrapper that command imports.
"""
from asgiref.wsgi import WsgiToAsgi

from app import app as flask_app
from app import boot

boot()
app = WsgiToAsgi(flask_app)
